#include <iostream>
#include <string>
#include <vector>
#include <libxml/HTMLparser.h>
#include <libxml/tree.h>
#include <libxml/uri.h>

// Function to traverse HTML and store href values in a vector
void traverseHTML(xmlNode *node, std::vector<std::string> &hrefs) {
    while (node) {
        if (node->type == XML_ELEMENT_NODE) {
            if (std::string(reinterpret_cast<const char *>(node->name)) == "a") {
                // Look for the "href" attribute
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
    htmlDocPtr doc = nullptr;  // Pointer to the HTML document

    // Parse the HTML file
    doc = htmlReadFile(filename.c_str(), nullptr, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (!doc) {
        std::cerr << "Error: Could not parse the HTML file: " << filename << std::endl;
        return;
    }

    // Get the root element of the document
    xmlNode *root_element = xmlDocGetRootElement(doc);

    // Traverse the HTML tree and collect href values
    traverseHTML(root_element, hrefs);

    // Free the document
    xmlFreeDoc(doc);
    xmlCleanupParser();
}

// Function to convert relative URLs to absolute URLs
void convertToAbsoluteUrls(std::vector<std::string> &hrefs, const std::string &baseUrl) {
    for (auto &href : hrefs) {
        // Convert std::string to xmlChar*
        xmlChar *absoluteUrl = xmlBuildURI(reinterpret_cast<const xmlChar *>(href.c_str()),
                                           reinterpret_cast<const xmlChar *>(baseUrl.c_str()));
        if (absoluteUrl) {
            href = reinterpret_cast<const char *>(absoluteUrl); // Update href with absolute URL
            xmlFree(absoluteUrl);
        }
    }
}

int main() {
    const std::string filename = "storage/output.html";
    const std::string baseUrl = "https://example.com/";
    std::vector<std::string> hrefs;

    // Parse the HTML file and extract href values
    parseHTML(filename, hrefs);

    // Convert relative URLs to absolute URLs
    convertToAbsoluteUrls(hrefs, baseUrl);

    // Print the absolute href values
    std::cout << "Absolute href values:\n";
    for (const auto &href : hrefs) {
        std::cout << href << std::endl;
    }

    return 0;
}
