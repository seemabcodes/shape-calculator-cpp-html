
#define _WIN32_WINNT 0x0600

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <winsock2.h>
#include <ws2tcpip.h>

using namespace std;

const int PORT = 8080;
const double PI = 3.14159265358979323846;

// CLASS: ShapeCalculator
class ShapeCalculator {
private:
    string shapeType;
    double dimensionA;
    double dimensionB;

    double area;
    double perimeter;
    double hypotenuse;

public:
    // CONSTRUCTOR
    ShapeCalculator(string type, double a, double b) {
        shapeType = type;
        setDimensionA(a);
        setDimensionB(b);

        area = 0;
        perimeter = 0;
        hypotenuse = 0;
    }

    // SETTERS
    void setShapeType(string type) {
        shapeType = type;
    }

    void setDimensionA(double a) {
        if (a > 0)
            dimensionA = a;
        else
            dimensionA = 1;
    }

    void setDimensionB(double b) {
        if (b > 0)
            dimensionB = b;
        else
            dimensionB = 1;
    }

    // GETTERS
    string getShapeType() const {
        return shapeType;
    }

    double getDimensionA() const {
        return dimensionA;
    }

    double getDimensionB() const {
        return dimensionB;
    }

    double getArea() const {
        return area;
    }

    double getPerimeter() const {
        return perimeter;
    }

    double getHypotenuse() const {
        return hypotenuse;
    }

    // MEMBER FUNCTION: calculation
    bool calculate() {
        if (shapeType == "rectangle") {
            area = dimensionA * dimensionB;
            perimeter = 2 * (dimensionA + dimensionB);
            hypotenuse = 0;
        }
        else if (shapeType == "circle") {
            area = PI * dimensionA * dimensionA;
            perimeter = 2 * PI * dimensionA;
            hypotenuse = 0;
        }
        else if (shapeType == "triangle") {
            area = (dimensionA * dimensionB) / 2.0;
            hypotenuse = sqrt(
                dimensionA * dimensionA +
                dimensionB * dimensionB
            );
            perimeter =
                dimensionA + dimensionB + hypotenuse;
        }
        else {
            return false;
        }

        return true;
    }

    // MEMBER FUNCTION: JSON result
    string getJSON() const {
        ostringstream out;
        out << setprecision(12);

        out << "\"area\":" << area
            << ",\"perimeter\":" << perimeter;

        if (shapeType == "triangle") {
            out << ",\"hypotenuse\":" << hypotenuse;
        }

        return out.str();
    }
};

// URL decoding
string urlDecode(const string& input) {
    string output;

    for (size_t i = 0; i < input.length(); i++) {
        if (input[i] == '+') {
            output += ' ';
        }
        else if (input[i] == '%' && i + 2 < input.length()) {
            string hex = input.substr(i + 1, 2);
            output += static_cast<char>(
                strtol(hex.c_str(), NULL, 16)
            );
            i += 2;
        }
        else {
            output += input[i];
        }
    }

    return output;
}

// Parse URL parameters
map<string, string> parseParams(const string& query) {
    map<string, string> params;
    stringstream stream(query);
    string item;

    while (getline(stream, item, '&')) {
        size_t pos = item.find('=');

        if (pos != string::npos) {
            string key = urlDecode(item.substr(0, pos));
            string value = urlDecode(item.substr(pos + 1));
            params[key] = value;
        }
    }

    return params;
}

// Read a positive number
bool getNumber(
    const map<string, string>& params,
    const string& key,
    double& value
) {
    map<string, string>::const_iterator it = params.find(key);

    if (it == params.end() || it->second.empty())
        return false;

    char* end = NULL;
    value = strtod(it->second.c_str(), &end);

    return end != it->second.c_str()
        && *end == '\0'
        && isfinite(value)
        && value > 0;
}

// Build JSON response
string handleRequest(const string& requestPath) {
    size_t qpos = requestPath.find('?');

    string route = requestPath.substr(0, qpos);
    string query = "";

    if (qpos != string::npos)
        query = requestPath.substr(qpos + 1);

    map<string, string> p = parseParams(query);

    map<string, string>::iterator typeIt = p.find("type");

    if (typeIt == p.end())
        return "{\"error\":\"Missing shape type\"}";

    string type = typeIt->second;

    double a = 0, b = 1, a2 = 0, b2 = 1;

    if (route == "/calculate") {
        if (!getNumber(p, "a", a))
            return "{\"error\":\"Enter a positive value for a\"}";

        if (type != "circle" && !getNumber(p, "b", b))
            return "{\"error\":\"Enter a positive value for b\"}";

        ShapeCalculator shape(type, a, b);

        if (!shape.calculate())
            return "{\"error\":\"Invalid shape type\"}";

        return "{" + shape.getJSON() + "}";
    }

    if (route == "/compare") {
        if (!getNumber(p, "a1", a) ||
            !getNumber(p, "a2", a2)) {
            return "{\"error\":\"Enter positive values for both shapes\"}";
        }

        if (type != "circle" &&
            (!getNumber(p, "b1", b) ||
             !getNumber(p, "b2", b2))) {
            return "{\"error\":\"Enter positive values for both shapes\"}";
        }

        ShapeCalculator shapeA(type, a, b);
        ShapeCalculator shapeB(type, a2, b2);

        if (!shapeA.calculate() || !shapeB.calculate())
            return "{\"error\":\"Invalid shape type\"}";

        ostringstream out;
        out << setprecision(12);

        out << "{\"shapeA\":{"
            << shapeA.getJSON()
            << "},\"shapeB\":{"
            << shapeB.getJSON()
            << "},\"comparison\":\"";

        double areaA = shapeA.getArea();
        double areaB = shapeB.getArea();

        if (fabs(areaA - areaB) < 1e-7)
            out << "Equal areas";
        else if (areaA > areaB)
            out << "Shape A has larger area";
        else
            out << "Shape B has larger area";

        out << "\"}";
        return out.str();
    }

    return "{\"error\":\"Unknown route\"}";
}

// HTTP response with CORS enabled
string makeResponse(const string& body, const string& status) {
    ostringstream response;

    response << "HTTP/1.1 " << status << "\r\n"
             << "Content-Type: application/json; charset=utf-8\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Access-Control-Allow-Methods: GET, OPTIONS\r\n"
             << "Access-Control-Allow-Headers: Content-Type\r\n"
             << "Connection: close\r\n"
             << "Content-Length: " << body.size()
             << "\r\n\r\n"
             << body;

    return response.str();
}

int main() {
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cout << "Winsock initialization failed.\n";
        return 1;
    }

    SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server == INVALID_SOCKET) {
        cout << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(server, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        cout << "Port 8080 busy hai. Purana backend band karo.\n";
        closesocket(server);
        WSACleanup();
        return 1;
    }

    if (listen(server, SOMAXCONN) == SOCKET_ERROR) {
        cout << "Server listen nahi kar saka.\n";
        closesocket(server);
        WSACleanup();
        return 1;
    }

    cout << "OOP Shape Calculator Backend is running!\n";
    cout << "Address: http://127.0.0.1:8080\n";
    cout << "HTML file browser mein open karo.\n";
    cout << "Backend chalte waqt yeh console band mat karna.\n\n";

    while (true) {
        SOCKET client = accept(server, NULL, NULL);

        if (client == INVALID_SOCKET)
            continue;

        char buffer[8192];
        int received = recv(client, buffer, sizeof(buffer) - 1, 0);

        if (received <= 0) {
            closesocket(client);
            continue;
        }

        buffer[received] = '\0';
        string request(buffer);

        size_t firstSpace = request.find(' ');
        size_t secondSpace = string::npos;

        if (firstSpace != string::npos)
            secondSpace = request.find(' ', firstSpace + 1);

        string method = "";
        string path = "/";

        if (firstSpace != string::npos && secondSpace != string::npos) {
            method = request.substr(0, firstSpace);
            path = request.substr(firstSpace + 1,
                                  secondSpace - firstSpace - 1);
        }

        
string body;
string status = "200 OK";

if (method == "OPTIONS") {
    body = "{}";
}
else if (method != "GET") {
    body = "{\"error\":\"Only GET is supported\"}";
    status = "405 Method Not Allowed";
}
else {
    body = handleRequest(path);

    if (body.find("\"error\"") != string::npos) {
        status = "400 Bad Request";
    }

    // Show request and result in Dev-C++ console
   
cout << "\n";
cout << "====================================\n";
cout << "       SHAPE CALCULATOR OUTPUT       \n";
cout << "====================================\n";

cout << "Shape Request: ";

if (path.find("type=rectangle") != string::npos)
    cout << "Rectangle\n";
else if (path.find("type=circle") != string::npos)
    cout << "Circle\n";
else if (path.find("type=triangle") != string::npos)
    cout << "Right-Angled Triangle\n";
else
    cout << "Unknown Shape\n";

cout << "------------------------------------\n";

if (body.find("\"error\"") != string::npos) {
    cout << "Status : Error\n";
    cout << "Message: " << body << "\n";
}
else {
    cout << "Status : Calculation Successful\n";
    cout << "Result : " << body << "\n";
}

cout << "====================================\n";
cout.flush();
 
}


        string response = makeResponse(body, status);

        send(client, response.c_str(),
             static_cast<int>(response.size()), 0);

        closesocket(client);
    }

    closesocket(server);
    WSACleanup();
    return 0;
}

