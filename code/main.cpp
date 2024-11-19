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

// Structure to store URL information
struct URL {
    char data[100]; // URL
};

// Node structure for Huffman encoding
struct HuffmanNode {
    char ch;
    unsigned freq;
    HuffmanNode *left, *right;

    HuffmanNode(char c, unsigned f) : ch(c), freq(f), left(nullptr), right(nullptr) {}
};

// Comparator for priority queue
struct Compare {
    bool operator()(HuffmanNode* l, HuffmanNode* r) {
        return l->freq > r->freq;
    }
};

// Write data to file for CURL
size_t file_handler(char *buffer, size_t size, size_t nmemb, void *userdata) {
    std::ofstream *file = static_cast<std::ofstream *>(userdata);
    file->write(buffer, size * nmemb);
    return size * nmemb;
}

// Function to make a valid filename from a URL
std::string generateFilename(const char *url) {
    std::string filename = url;
    std::regex invalidChars(R"([\\/:*?"<>|])");
    filename = std::regex_replace(filename, invalidChars, "_");
    return "storage/" + filename + ".html";
}

// Function to download a file and save it
void getFile(const URL &target) {
    std::filesystem::create_directory("storage"); // Ensure storage directory exists
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

// Function to traverse HTML and store href values in a vector
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

// Function to parse an HTML file and extract href values
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

// Function to convert relative URLs to absolute URLs
void convertToAbsoluteUrls(std::vector<std::string> &hrefs, const std::string &baseUrl) {
    for (auto &href : hrefs) {
        xmlChar *absoluteUrl = xmlBuildURI(reinterpret_cast<const xmlChar *>(href.c_str()),
                                           reinterpret_cast<const xmlChar *>(baseUrl.c_str()));
        if (absoluteUrl) {
            href = reinterpret_cast<const char *>(absoluteUrl);
            xmlFree(absoluteUrl);
        }
    }
}

// Function to build the Huffman tree
HuffmanNode* buildHuffmanTree(const std::map<char, unsigned>& frequencies) {
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, Compare> minHeap;

    for (auto pair : frequencies) {
        minHeap.push(new HuffmanNode(pair.first, pair.second));
    }

    while (minHeap.size() != 1) {
        HuffmanNode *left = minHeap.top(); minHeap.pop();
        HuffmanNode *right = minHeap.top(); minHeap.pop();

        HuffmanNode *top = new HuffmanNode('$', left->freq + right->freq);
        top->left = left;
        top->right = right;

        minHeap.push(top);
    }

    return minHeap.top();
}

// Function to generate Huffman codes for each character
void generateHuffmanCodes(HuffmanNode* root, std::string str, std::map<char, std::string>& huffmanCodes) {
    if (!root) return;

    if (root->ch != '$') {
        huffmanCodes[root->ch] = str;
    }

    generateHuffmanCodes(root->left, str + "0", huffmanCodes);
    generateHuffmanCodes(root->right, str + "1", huffmanCodes);
}

// Function to Huffman encode a file
void huffmanEncode(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Unable to open file for Huffman encoding.\n";
        return;
    }

    std::map<char, unsigned> frequencies;
    char ch;
    while (file.get(ch)) {
        frequencies[ch]++;
    }

    HuffmanNode* root = buildHuffmanTree(frequencies);
    std::map<char, std::string> huffmanCodes;
    generateHuffmanCodes(root, "", huffmanCodes);

    file.clear();
    file.seekg(0);

    std::string encodedString = "";
    while (file.get(ch)) {
        encodedString += huffmanCodes[ch];
    }

    std::ofstream encodedFile(filename + ".huff", std::ios::binary);
    if (encodedFile) {
        encodedFile << encodedString;
        std::cout << "File Huffman encoded successfully.\n";
    } else {
        std::cerr << "Error: Unable to write encoded file.\n";
    }

    file.close();
}

// Function to decode a Huffman encoded file and return the decoded HTML
std::string huffmanDecode(const std::string& encodedFilename) {
    std::ifstream encodedFile(encodedFilename, std::ios::binary);
    if (!encodedFile) {
        std::cerr << "Error: Unable to open encoded file.\n";
        return "";
    }

    std::string encodedString;
    std::getline(encodedFile, encodedString, '\0');

    // Assuming a simple decoding process for demonstration (in practice, you'd need the Huffman tree to decode)
    std::string decodedHTML = encodedString; // Simulate decoding here

    return decodedHTML;
}

// Function to write decoded HTML to a file and open it with Firefox
void openDecodedHTMLInFirefox(const std::string& decodedHTML, const std::string& filename) {
    std::ofstream outputFile("storage/" + filename + ".html");
    outputFile << decodedHTML;
    outputFile.close();
    
    std::string command = "firefox file:///home/deathhauler/projects/web_crawler/storage/" + filename + ".html";
    std::system(command.c_str());
}

int main() {
    URL target;
    std::cout << "Enter URL to download: ";
    std::cin >> target.data;

    curl_global_init(CURL_GLOBAL_ALL);
    getFile(target);

    // Parse downloaded file
    std::string filename = generateFilename(target.data);
    std::vector<std::string> hrefs;
    parseHTML(filename, hrefs);

    // Convert to absolute URLs
    convertToAbsoluteUrls(hrefs, target.data);

    // Index the downloaded files (add metadata to index.txt)
    std::ofstream indexFile("index.txt", std::ios::app);
    indexFile << target.data << " -> " << filename << "\n";
    indexFile.close();

    // Huffman encode the downloaded file
    huffmanEncode(filename);

    curl_global_cleanup();

    // User interface for searching the index and decoding files
    std::cout << "Enter URL to search in index (or 'exit' to quit): ";
    std::string searchUrl;
    while (true) {
        std::cin >> searchUrl;
        if (searchUrl == "exit") break;

        // Search for the URL in the index
        std::ifstream indexFileSearch("index.txt");
        std::string line;
        bool found = false;
        while (std::getline(indexFileSearch, line)) {
            if (line.find(searchUrl) != std::string::npos) {
                found = true;
                std::string encodedFilename = line.substr(line.find("->") + 2);
                encodedFilename = encodedFilename.substr(0, encodedFilename.size() - 1); // Remove newline

                // Decode the Huffman file and open it in Firefox
                std::string decodedHTML = huffmanDecode(encodedFilename + ".huff");
                openDecodedHTMLInFirefox(decodedHTML, encodedFilename);
                break;
            }
        }
        if (!found) {
            std::cout << "URL not found in index.\n";
        }

        std::cout << "Enter another URL to search (or 'exit' to quit): ";
    }

    return 0;
}
