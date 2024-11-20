#include <iostream>
#include <vector>
#include <cstring>
#include <fstream>
#include <string>
#include <curl/curl.h>
#include <filesystem>
#include <libxml/HTMLparser.h>
#include <libxml/tree.h>
#include <libxml/uri.h>
#include <queue>
#include <set>
#include <sstream>
#include <openssl/sha.h>
#include <iomanip>
#include <regex>
#include <libxml/uri.h>
enum ftype
{
    HTML,
    CSS,
    JS
};

struct URL
{
    char data[1000];
};

void printer(std::vector<std::string> a)
{
    for (int i = 0; i < a.size(); i++)
    {
        std::cout << a[i] << " ";
    }
}

std::string sanitize(std::string a)
{
    std::regex restrictedChars(R"([\\/:*?"<>|])");
    return std::regex_replace(a, restrictedChars, "_");
}

std::string hashURL(const std::string &url)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, url.c_str(), url.size());
    SHA256_Final(hash, &sha256);

    std::ostringstream hexStream;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        hexStream << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return hexStream.str();
}

size_t file_handler(char *buffer, size_t size, size_t nmemb, void *userdata)
{
    std::ofstream *file = static_cast<std::ofstream *>(userdata);
    file->write(buffer, size * nmemb);
    return size * nmemb;
}

std::string generateFilename(const char *url)
{
    std::string filename = hashURL(url);
    return "storage/" + filename;
}

void traverse(xmlNode *node, std::vector<std::string> &hrefs, std::vector<std::string> &css, std::vector<std::string> &js, const std::string &baseUri)
{
    while (node)
    {
        if (node->name)
        {
            std::cout << reinterpret_cast<const char *>(node->name) << "\n";
        }

        if (node->type == XML_ELEMENT_NODE)
        {
            std::string nodeName = reinterpret_cast<const char *>(node->name);

            if (nodeName == "a")
            {
                for (xmlAttr *attr = node->properties; attr; attr = attr->next)
                {
                    if (std::string(reinterpret_cast<const char *>(attr->name)) == "href")
                    {
                        xmlChar *value = xmlNodeListGetString(node->doc, attr->children, 1);
                        if (value)
                        {
                            xmlChar *absoluteUri = xmlBuildURI(value, reinterpret_cast<const xmlChar *>(baseUri.c_str()));
                            if (absoluteUri)
                            {
                                hrefs.emplace_back(reinterpret_cast<const char *>(absoluteUri));
                                xmlFree(absoluteUri);
                            }
                            else
                                hrefs.emplace_back(reinterpret_cast<const char *>(value));
                            xmlFree(value);
                        }
                    }
                }
            }
            else if (nodeName == "link")
            {
                // Check for rel="stylesheet"
                std::string rel;
                for (xmlAttr *attr = node->properties; attr; attr = attr->next)
                {
                    if (std::string(reinterpret_cast<const char *>(attr->name)) == "rel")
                    {
                        rel = reinterpret_cast<const char *>(attr->children ? (const char *)attr->children->content : "");
                    }

                    if (std::string(reinterpret_cast<const char *>(attr->name)) == "href")
                    {
                        xmlChar *value = xmlNodeListGetString(node->doc, attr->children, 1);
                        if (value)
                        {
                            // Only process link if rel="stylesheet"
                            if (rel == "stylesheet")
                            {
                                xmlChar *absoluteUri = xmlBuildURI(value, reinterpret_cast<const xmlChar *>(baseUri.c_str()));
                                if (absoluteUri)
                                {
                                    css.emplace_back(reinterpret_cast<const char *>(absoluteUri));
                                    xmlFree(absoluteUri);
                                }
                                else
                                    css.emplace_back(reinterpret_cast<const char *>(value));
                            }
                            xmlFree(value);
                        }
                    }
                }
            }
            else if (nodeName == "script")
            {
                for (xmlAttr *attr = node->properties; attr; attr = attr->next)
                {
                    if (std::string(reinterpret_cast<const char *>(attr->name)) == "src")
                    {
                        xmlChar *value = xmlNodeListGetString(node->doc, attr->children, 1);
                        if (value)
                        {
                            xmlChar *absoluteUri = xmlBuildURI(value, reinterpret_cast<const xmlChar *>(baseUri.c_str()));
                            if (absoluteUri)
                            {
                                js.emplace_back(reinterpret_cast<const char *>(absoluteUri));
                                xmlFree(absoluteUri);
                            }
                            else
                                js.emplace_back(reinterpret_cast<const char *>(value));
                            xmlFree(value);
                        }
                    }
                }
            }
        }

        traverse(node->children, hrefs, css, js, baseUri);
        node = node->next;
    }
}

void parse(const std::string &filename, std::vector<std::string> &hrefs, std::vector<std::string> &css, std::vector<std::string> &js, const URL &startURL)
{
    htmlDocPtr doc = htmlReadFile(filename.c_str(), nullptr, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (!doc)
    {
        std::cerr << "Error: Could not parse the HTML file: " << filename << std::endl;
        return;
    }

    xmlNode *root_element = xmlDocGetRootElement(doc);
    traverse(root_element, hrefs, css, js, startURL.data);
    std::cout << "links";
    // printer(hrefs);
    std::cout << "css";
    printer(css);
    std::cout << "js";
    printer(js);
    std::cout << std::endl;
    xmlFreeDoc(doc);
    xmlCleanupParser();
}

void getFile(const URL &target, const std::string &sessionFolder, ftype type)
{
    std::filesystem::create_directory(sessionFolder);
    std::string filename;
    switch (type)
    {
    case HTML:
    {
        filename = sessionFolder + "/" + sanitize(target.data) + ".html";
        break;
    }
    case CSS:
    {
        filename = sessionFolder + "/" + sanitize(target.data) + ".css";
        break;
    }
    case JS:
    {
        filename = sessionFolder + "/" + sanitize(target.data) + ".js";
        break;
    }
    default:
        break;
    }

    std::ofstream file(filename, std::ios::binary);
    if (!file)
    {
        std::cerr << "Error: Unable to open file " << filename << " for writing.\n";
        return;
    }

    CURL *handle = curl_easy_init();
    if (!handle)
    {
        std::cerr << "Error: Unable to initialize CURL.\n";
        return;
    }

    curl_easy_setopt(handle, CURLOPT_URL, target.data);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, file_handler);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &file);
    CURLcode res = curl_easy_perform(handle);

    if (res != CURLE_OK)
        std::cerr << "Error: CURL request failed.\n";
    curl_easy_cleanup(handle);
    file.close();

    std::cout << "Page saved to: " << filename << "\n";
}

void runHTML(URL *input)
{
    std::string filename = generateFilename(input->data);
    std::string command = "firefox file:///home/deathhauler/projects/web_crawler/" + filename;
    std::system(command.c_str());
}

std::string makeAbsoluteURL(const std::string &baseURL, const std::string &relativeURL)
{
    xmlChar *absolute = xmlBuildURI(reinterpret_cast<const xmlChar *>(relativeURL.c_str()),
                                    reinterpret_cast<const xmlChar *>(baseURL.c_str()));
    std::string absoluteURL;
    if (absolute)
    {
        absoluteURL = reinterpret_cast<const char *>(absolute);
        xmlFree(absolute);
    }
    return absoluteURL;
}

void crawl(URL startURL, int depth, std::set<std::string> &visited, const std::string &sessionFolder)
{
    std::queue<std::pair<std::string, int>> queue;
    queue.push({startURL.data, 0});

    while (!queue.empty())
    {
        auto [currentURL, currentDepth] = queue.front();
        queue.pop();

        if (visited.find(currentURL) != visited.end() || currentDepth > depth)
        {
            continue;
        }
        visited.insert(currentURL);
        std::cout << "Crawling: " << currentURL << " (Depth: " << currentDepth << ")\n";

        std::string filename = sessionFolder + "/" + sanitize(currentURL) + ".html";
        std::vector<std::string> hrefs;
        std::vector<std::string> css;
        std::vector<std::string> js;
        parse(filename, hrefs, css, js, startURL);

        URL target;
        strncpy(target.data, currentURL.c_str(), sizeof(target.data) - 1);
        target.data[sizeof(target.data) - 1] = '\0';

        getFile(target, sessionFolder, HTML);
        for (int i = 0; i < css.size(); i++)
        {
            strncpy(target.data, css[i].c_str(), sizeof(target.data) - 1);
            target.data[sizeof(target.data) - 1] = '\0';
            getFile(target, sessionFolder, CSS);
        }
        for (int i = 0; i < js.size(); i++)
        {
            strncpy(target.data, js[i].c_str(), sizeof(target.data) - 1);
            target.data[sizeof(target.data) - 1] = '\0';
            getFile(target, sessionFolder, CSS);
        }

        for (const auto &link : hrefs)
        {
            std::string absoluteLink = makeAbsoluteURL(currentURL, link);

            if (!absoluteLink.empty() && visited.find(absoluteLink) == visited.end())
            {
                queue.push({absoluteLink, currentDepth + 1});
            }
        }
    }
}

int main()
{
    URL target;
    std::cout << "Enter URL to start crawling: ";
    // std::cin >> target.data;
    strcpy(target.data, "https://quotes.toscrape.com/");
    int depth;
    std::cout << "Enter crawl depth: ";
    // std::cin >> depth;
    depth = 0;
    std::set<std::string> visited;

    // Remove restricted characters from the URL
    std::string safeName = sanitize(target.data);
    std::string sessionFolder = "storage/" + safeName;
    std::filesystem::create_directory(sessionFolder);

    // Initialize cURL globally
    curl_global_init(CURL_GLOBAL_ALL);

    // Start crawling
    crawl(target, depth, visited, sessionFolder);

    // Cleanup cURL globally
    curl_global_cleanup();

    std::cout << "All pages saved in folder: " << sessionFolder << "\n";
    // TODO Add host

    return 0;
}