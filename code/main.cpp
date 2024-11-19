
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <curl/curl.h>
#include <regex>
#include <filesystem>
#include <libxml/HTMLparser.h>
#include <libxml/tree.h>
#include <libxml/uri.h>
#include <map>
#include <queue>
#include <bitset>
#include <sstream>
#include <openssl/sha.h>

struct URL
{
    char data[100];
};

std::string hashURL(const std::string &url) {
    // Initialize a SHA256 context
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    // Hash the input URL
    SHA256_Update(&sha256, url.c_str(), url.size());
    SHA256_Final(hash, &sha256);

    // Convert hash to hexadecimal string
    std::ostringstream hexStream;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hexStream << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return hexStream.str();
}

size_t file_handler(char *buffer, size_t size, size_t nmemb, void *userdata) {
    std::ofstream *file = static_cast<std::ofstream *>(userdata);
    file->write(buffer, size * nmemb);
    return size * nmemb;
}

std::string generateFilename(const char *url) {
    std::string filename = hashURL(url);
    return "storage/" + filename;
}

void traverseHTML(xmlNode *node, std::vector<std::string> &hrefs) {
    while (node) {
        if (node->type == XML_ELEMENT_NODE) {
            if (std::string(reinterpret_cast<const char *>(node->name)) == "a") {
                for (xmlAttr *attr = node->properties; attr; attr = attr->next) {
                    if (std::string(reinterpret_cast<const char *>(attr->name)) == "href") {
                        xmlChar *value = xmlNodeListGetString(node->doc, attr->children, 1);
                        if (value) {
                            hrefs.emplace_back(reinterpret_cast<const char *>(value));
                            xmlFree(value);
                        }
                    }
                }
            }
        }
        traverseHTML(node->children, hrefs);
        node = node->next;
    }
}


void parseHTML(const std::string &filename, std::vector<std::string> &hrefs) {
    htmlDocPtr doc = htmlReadFile(filename.c_str(), nullptr, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (!doc) {
        std::cerr << "Error: Could not parse the HTML file: " << filename << std::endl;
        return;
    }

    xmlNode *root_element = xmlDocGetRootElement(doc);
    traverseHTML(root_element, hrefs);
    xmlFreeDoc(doc);
    xmlCleanupParser();
}


void getFile(const URL &target int mode=0) {
    std::filesystem::create_directory("storage");
    std::string filename = generateFilename(target.data);

    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Unable to open file " << filename << " for writing.\n";
        return;
    }

    CURL *handle = curl_easy_init();
    if (!handle) {
        std::cerr << "Error: Unable to initialize CURL.\n";
        return;
    }

    curl_easy_setopt(handle, CURLOPT_URL, target.data);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, file_handler);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &file);
    CURLcode res = curl_easy_perform(handle);

    if (res != CURLE_OK) {
        std::cerr << "Error: CURL request failed.\n";
    }

    curl_easy_cleanup(handle);
    file.close();
}

void runHTML (URL* input)
{
    std::string filename = generateFilename(input->data);
    std::string command = "firefox file:///home/deathhauler/projects/web_crawler/" + filename;
    std::system(command.c_str());
}


int main()
{
    URL target;
    std::cout << "Enter URL to download: ";
    std::cin >> target.data;

    // get File
    curl_global_init(CURL_GLOBAL_ALL);
    getFile(target);
    curl_global_cleanup();

    // Parse for CSS (In General Parse)
    std::vector<std::string> hrefs;
    parseHTML(target.data , hrefs);

    // Run File on Firefox
    runHTML(&target);
    return 0;
}