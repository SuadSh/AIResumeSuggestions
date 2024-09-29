#include <iostream> // I/O
#include <fstream> // File Opening
#include <array> // Buffer array
#include <string> // String
#include <nlohmann/json.hpp> // JSON Managment
#include <curl/curl.h> // Sending requests to the AI

using json = nlohmann::json; 

// Error for cleaner code
void error(const std::string err){
    std::cerr<<err;
    exit(-1);
}

// Function to read the json file data
json readData(const std::string &filename) {
    // Open file
    std::ifstream file(filename);
    json jsonData;
    
    // Check if opened
    if (file.is_open()) {
        file >> jsonData;
    } else {
        error("Couldn't open JSON file");
    }
    file.close();

    // Return retrieved data
    return jsonData;
}

// In order to use the python script
std::string pythonScrape(const std::string &url){
    std::array<char, 4096> buffer; // Buffer of 4096 characters
    const std::string pythonCommand = "py ../scraper.py "+ url; // The command with the url as an argument to be passed to the script
    std::string data;

    // Create a unique ptr (since it will point to a dynamic object) with file and decltype as template parameters
    // File is for reading the output, decltype is for determing the type of pclose so it can be properly used from the ptr 
    std::unique_ptr<FILE,decltype(&pclose)> pipe(popen(pythonCommand.c_str(), "r"), pclose);
    // Pipe is the pointers name and we use popen to open the pipe, we pass the pythoncommand as an argument after converting it to c_str
    // the second argument determins the use of the pipe (read) and pclose is passed with popen so it closes when the script terminates

    // Ensure the pipe opened correctly
    if(!pipe){
        error("Failed to open the pipe to the python script");
    }

    // While fgets has data to receive from the python script append it to the data variable
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        data += buffer.data(); 
    }
    
    return data;
}

// WriteCallBack will be used to send the data to the AI model
std::size_t WriteCallback(void* contents, std::size_t size, std::size_t elemNum, std::string* userprompt) {
    std::size_t totalSize = size * elemNum; // Calculate the total size of the prompt by the size and the number of elements
    userprompt->append((char*)contents, totalSize); // Append the user prompt by converting the contents to char and as a second argument send the total size of the prompt
    return totalSize;
}

// AI Handling
std::string callOpenAI(const std::string& prompt, const std::string& apiKey) {
    CURL* curl; // Curl object
    CURLcode res; // Result curl code
    std::string readBuffer; // Buffer for the receiving data to be stored

    // Request body (from openai examples)
    json requestBody = {
        {"model", "gpt-3.5-turbo"},
        {"messages", {
            {{"role", "user"}, {"content", prompt}}
        }},
        {"temperature", 0.1} // Temperature preferably low for the resume to be relevant
    };

    // Initialize the curl object 
    curl_global_init(CURL_GLOBAL_DEFAULT); 
    curl = curl_easy_init();

    if(curl) {
        // Specify the url that curl will send the requests
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");

        // Header management
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, nullptr); // Sets headers
        struct curl_slist *headers = nullptr; // Creates a list of heading since we need the content type and api authorization
        headers = curl_slist_append(headers, "Content-Type: application/json"); // Specifies that the headers conentent will be in json (requestBody)
        headers = curl_slist_append(headers, ("Authorization: Bearer " + apiKey).c_str()); // Passes the api key to a header
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Pass the headers list to the curl session

        std::string jsonStr = requestBody.dump(); // Request(json) to string 
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());

        // Data received handling 
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); // Handle the received data through previous function
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer); // Storing the response to readBuffer

        res = curl_easy_perform(curl); // Sending the request
        if (res != CURLE_OK) { // If any error occured with the request exit the program
            error("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
        }

        // Cleanup
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    // Final cleanup since we got the data we needed
    curl_global_cleanup();

    // Encapsuled in a try-catch because when exceeding the quota of the free plan (since the job descriptions have large amount of tokens) it would crash
    try {

        // Parse the response
        json response = json::parse(readBuffer);

        // Check if received response is in correct form
        if (response.contains("choices") && !response["choices"].empty() &&
            response["choices"][0].contains("message") && 
            response["choices"][0]["message"].contains("content") &&
            response["choices"][0]["message"]["content"].is_string()) {
            
            // Extract the response content
            return response["choices"][0]["message"]["content"];
        } else {
            error("Unexpected response structure: " + readBuffer);
        }
    } catch (const json::exception& e) {
        error("JSON parsing error: " + std::string(e.what()));
    }

    return "";
}

int main(int argc,char **argv){
    std::string url; // URL Link variable
    json info; // Json to hold the candidate info 
    std::string jobData; // Job data variable
    const std::string apiKey = "apiKey"; // OpenAi api key

    // Get the info from the JSON file
    info = readData("../my_info.json");
    
    // Message to the user
    std::cout<<"Provide the job URL in order to start the process: \n";
    std::cin>>url;

    // Get the job data through the python program
    jobData = pythonScrape(url);

    std::string prompt = "Given the following JSON data: " + info.dump() + "Alter the data in order emphasize only the relevant to the following job " + jobData;
    std::cout<<"AI Response: "<<callOpenAI(prompt,apiKey);

    std::cin>>url;
    return 0;
}