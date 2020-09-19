#include "Audio.hpp"
#include "Log.hpp"
#include <switch.h>
#include <unordered_map>

namespace NX {
    // Helper to log messages
    void logError(const std::string & service, const Result & rc) {
        Log::writeError("[NX] Failed to initialize " + service + ": " + std::to_string(rc));
    }

    // Variables indicating if each service was initialized
    static bool audrenInitialized = false;
    static bool fsInitialized = false;
    static bool gpioInitialized = false;
    static bool pscmInitialized = false;
    static bool smInitialized = false;
    static bool socketInitialized = false;

    // Starts all needed services
    bool startServices() {
        // Prevent starting twice
        if (audrenInitialized || fsInitialized || gpioInitialized || pscmInitialized || smInitialized || socketInitialized) {
            return true;
        }
        Result rc;

        // SM
        rc = smInitialize();
        if (R_SUCCEEDED(rc)) {
            smInitialized = true;

        } else {
            return false;
        }

        // FS
        rc = fsInitialize();
        if (R_SUCCEEDED(rc)) {
            fsdevMountSdmc();
            fsInitialized = true;

        } else {
            return false;
        }

        // Open log file
        Log::openFile("/switch/TriPlayer/sysmodule.log", Log::Level::Warning);

        // Set system version
        rc = setsysInitialize();
        if (R_SUCCEEDED(rc)) {
            SetSysFirmwareVersion fw;
            setsysGetFirmwareVersion(&fw);
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
            setsysExit();

        } else {
            logError("system version", rc);
        }

        // Sockets (uses small buffers)
        constexpr SocketInitConfig socketCfg = {
            .bsdsockets_version = 1,

            .tcp_tx_buf_size = 0x1000,
            .tcp_rx_buf_size = 0x1000,
            .tcp_tx_buf_max_size = 0x3000,
            .tcp_rx_buf_max_size = 0x3000,

            .udp_tx_buf_size = 0x0,
            .udp_rx_buf_size = 0x0,

            .sb_efficiency = 1,
        };
        rc = socketInitialize(&socketCfg);
        if (R_SUCCEEDED(rc)) {
            socketInitialized = true;

        } else {
            logError("socket", rc);
            return false;
        }

        // GPIO
        rc = gpioInitialize();
        if (R_SUCCEEDED(rc)) {
            gpioInitialized = true;

        } else {
            logError("gpio", rc);
        }

        // PSC
        rc = pscmInitialize();
        if (R_SUCCEEDED(rc)) {
            pscmInitialized = true;

        } else {
            logError("pscm", rc);
        }

        // Audio
        rc = audrenInitialize(&audrenCfg);
        if (R_SUCCEEDED(rc)) {
            Audio::getInstance();
            audrenStartAudioRenderer();
            audrenInitialized = true;

        } else {
            logError("audio", rc);
            return false;
        }

        // We don't care if gpio or pscm don't initialize
        return true;
    }

    // Stops all started services (in reverse order)
    void stopServices() {
        // PSC
        if (pscmInitialized) {
            pscmExit();
            pscmInitialized = false;
        }

        // GPIO
        if (gpioInitialized) {
            gpioExit();
            gpioInitialized = false;
        }

        // Sockets
        if (socketInitialized) {
            socketExit();
            socketInitialized = false;
        }

        // Audio
        if (audrenInitialized) {
            audrenStopAudioRenderer();
            delete Audio::getInstance();
            audrenExit();
            audrenInitialized = false;
        }

        // Close log
        Log::closeFile();

        // FS
        if (fsInitialized) {
            fsdevUnmountAll();
            fsExit();
            fsInitialized = false;
        }

        // SM
        if (smInitialized) {
            smExit();
            smInitialized = false;
        }
    }

    namespace Gpio {
        static bool gpioPrepared = false;       // Set true if ready to poll pad
        static GpioPadSession gpioSession;      // Current session used to read pad
        static GpioValue gpioLastValue;         // Last read value (low if plugged in, high if not)

        // Open session for headset pad
        bool prepare() {
            // Don't prepare if service not initialized
            if (!gpioInitialized) {
                logError("gpio pad", -1);
                return false;
            }

            // Prevent preparing twice
            if (gpioPrepared) {
                return true;
            }

            // Open for GPIO pad 0x15, which indicates if headset is connected
            gpioLastValue = GpioValue_High;
            Result rc = gpioOpenSession(&gpioSession, (GpioPadName)0x15);
            if (R_SUCCEEDED(rc)) {
                gpioPrepared = true;
                return true;
            }

            logError("gpio pad", -2);
            return false;
        }

        // Close opened session
        void cleanup() {
            if (gpioPrepared) {
                gpioPadClose(&gpioSession);
                gpioPrepared = false;
            }
        }

        // Return if the headset was just unplugged
        bool headsetUnplugged() {
            // Don't attempt to check if not prepared
            if (!gpioPrepared) {
                return false;
            }

            // Get current value
            GpioValue currentValue;
            Result rc = gpioPadGetValue(&gpioSession, &currentValue);
            if (R_FAILED(rc)) {
                return false;
            }

            // If we have changed from Low to High, the headset was unplugged
            bool unplugged = false;
            if (gpioLastValue == GpioValue_Low && currentValue == GpioValue_High) {
                unplugged = true;
            }
            gpioLastValue = currentValue;
            return unplugged;
        }
    };

    namespace Psc {
        constexpr u16 pscDependencies[] = {PscPmModuleId_Audio};    // Our dependencies
        constexpr PscPmModuleId pscModuleId = (PscPmModuleId)690;   // Our ID
        static PscPmModule pscModule;                               // Module to listen for events with
        static bool pscPrepared = false;                            // Set true if ready to handle event

        // Create module to listen with
        bool prepare() {
            // Don't prepare if service not initialized
            if (!pscmInitialized) {
                logError("psc module", -1);
                return false;
            }

            // Prevent preparing twice
            if (pscPrepared) {
                return true;
            }

            // Create module
            Result rc = pscmGetPmModule(&pscModule, pscModuleId, pscDependencies, sizeof(pscDependencies)/sizeof(u16), true);
            if (R_SUCCEEDED(rc)) {
                pscPrepared = true;
                return true;
            }

            logError("psc module", -2);
            return false;
        }

        // Delete created module
        void cleanup() {
            if (pscPrepared) {
                pscPmModuleFinalize(&pscModule);
                pscPmModuleClose(&pscModule);
                pscPrepared = false;
            }
        }

        bool enteringSleep(const size_t ms) {
            size_t ns = 1000000 * ms;

            // Don't wait for event if not prepared
            if (!pscPrepared) {
                svcSleepThread(ns);
                return false;
            }

            // Wait for an event
            Result rc = eventWait(&pscModule.event, ns);
            if (R_VALUE(rc) == KERNELRESULT(TimedOut)) {
                return false;

            } else if (R_VALUE(rc) == KERNELRESULT(Cancelled)) {
                svcSleepThread(ns);
                return false;
            }

            // If there's an event, fetch the state
            PscPmState eventState;
            u32 flags;
            rc = pscPmModuleGetRequest(&pscModule, &eventState, &flags);
            if (R_FAILED(rc)) {
                return false;
            }

            // Check if we're entering sleep
            bool enteringSleep = false;
            if (eventState == PscPmState_ReadySleep) {
                enteringSleep = true;
            }
            pscPmModuleAcknowledge(&pscModule, eventState);
            return enteringSleep;
        }
    };

    namespace Thread {
        constexpr size_t threadStackSize = 0x5000;                    // Size of each thread's stack (bytes)
        static std::unordered_map<std::string, ::Thread> threads;     // Map from name/id to thread object

        bool create(const std::string & id, void(*func)(void *), void * arg) {
            // Don't start if thread exists
            if (threads.count(id) > 0) {
                return false;
            }

            // Create thread
            ::Thread thread;
            Result rc = threadCreate(&thread, func, arg, nullptr, threadStackSize, 0x2C, -2);
            if (R_FAILED(rc)) {
                logError("thread (" + id + ")", rc);
                return false;
            }

            // Start thread execution
            rc = threadStart(&thread);
            if (R_FAILED(rc)) {
                logError("thread (" + id + ")", rc);
            }
            threads[id] = thread;
            return true;
        }

        void join(const std::string & id) {
            // Check if thread exists
            if (threads.count(id) == 0) {
                return;
            }

            // Wait for thread to finish
            ::Thread thread = threads[id];
            threads.erase(id);
            threadWaitForExit(&thread);
            threadClose(&thread);
        }
    }
};