#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;


// Callback function to write data from libcurl into a string
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) 
{
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

std::string smhi() 
{
    // Läs in data från denna länken ↓
    const std::string url = "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/15.5869/lat/56.1612/data.json";
    std::string buffer;

    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to init curl" << std::endl;
        return 0;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "C++ SMHI Client/1.0");
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr << "Curl error: " << curl_easy_strerror(res) << std::endl;
        return 0;
    }

    return buffer;
}

json parse_json(std::string buffer) 
{
    // Konvertera text till json
    try 
    {
        auto data = json::parse(buffer);
        return data;
    } 
    catch(const std::exception& e)
    {
        return "Json parse error!";
    }

    return 0;
}

int main()
{
    // Formatera och skriv ut 
    json weather_data_json = parse_json(smhi());
    json timeSeries = weather_data_json["timeSeries"];

    std::cout << "Karlskrona väder idag:\n";
    for (size_t i = 0; i < 8 && i < timeSeries.size(); ++i) 
    {
        auto t = timeSeries[i]["validTime"];
        double temp = 0.0, precip = 0.0;

        for (const auto& p : timeSeries[i]["parameters"]) 
        {
            if (p["name"] == "t")
            {
                temp = p["values"][0];
            }
            if (p["name"] == "pmean") 
            {
                precip = p["values"][0];
            }
        }

        std::cout << t << " | " << temp << "°C, "<< precip << " mm precipitation\n";

    }
    return 0;
}
