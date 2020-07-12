#include "nlohmann/json.hpp"
#include "utils/Curl.hpp"
#include "utils/metadata/AudioDB.hpp"

// Format of API request
#define API_REQUEST "https://theaudiodb.com/api/v1/json/1/artist-mb.php?i="

namespace Metadata::AudioDB {
    std::vector<unsigned char> getArtistImage(const std::string & id) {
        std::vector<unsigned char> v;

        // The url will be escaped/encoded by curl
        std::string url = API_REQUEST + id;
        std::string response = "";
        bool success = Utils::Curl::downloadToString(url, response);
        if (!success || response.empty()) {
            return v;
        }

        // Convert to JSON object and get image URL
        url = "";
        nlohmann::json j = nlohmann::json::parse(response);
        if (j["artists"] != nullptr) {
            if (j["artists"]["strArtistThumb"] != nullptr) {
                url = j["artists"]["strArtistThumb"].get<std::string>();
            }
        }
        if (url.empty()) {
            return v;
        }

        // Download image
        success = Utils::Curl::downloadToBuffer(url, v);
        if (!success) {
            v.clear();
        }

        return v;
    }
};