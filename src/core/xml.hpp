#include <lightwave/core.hpp>

#include <filesystem>
#include <string>

namespace lightwave {

class XMLParser {
    // Note: The parser is not standard-conform and only parses basic XML

public:
    struct SourceLocation {
        std::string filename;
        int line   = 1;
        int column = 1;
    };

    struct Delegate {
        virtual void open(const std::string &tag,
                          const SourceLocation &loc)     = 0;
        virtual void enter()                             = 0;
        virtual void close()                             = 0;
        virtual void attribute(const std::string &name,
                               const std::string &value) = 0;
        virtual void stop()                              = 0;
    };

private:
    Delegate &m_delegate;
    std::istream *m_stream;
    SourceLocation m_loc;

public:
    XMLParser(Delegate &delegate, std::istream &stream);
    XMLParser(Delegate &delegate, const std::filesystem::path &path);

private:
    void parse();
    int peek();
    int get();
    void expectToken(char token);
    std::string readIdentifier();
    std::string readString();
    void readComment();
    void skipWhitespace();
    bool readNode(std::string enclosingTag);
};

} // namespace lightwave
