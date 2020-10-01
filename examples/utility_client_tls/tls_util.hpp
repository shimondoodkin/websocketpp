#ifndef WEBSOCKETPP_TLS_UTIL_HPP
#define WEBSOCKETPP_TLS_UTIL_HPP

#ifdef _MSC_VER 
//not #if defined(_WIN32) || defined(_WIN64) because we have strncasecmp in mingw
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#pragma warning( push )
#pragma warning( disable : 26812 )
#pragma warning( disable : 26495 )
#pragma warning( disable : 6255 )
#pragma warning( disable : 6387 )
#pragma warning( disable : 6031 )
#pragma warning( disable : 6001 )
#pragma warning( disable : 6001 )
#pragma warning( disable : 26439 )
#pragma warning( disable : 26819 )
#pragma warning( disable : 26451 )
#pragma warning( disable : 6258 )
#pragma warning( disable : 26498 )
#pragma warning( disable : 26498 )
#pragma warning( disable : 28251 )
#pragma warning( disable : 4267 )
#pragma warning( disable : 6385 )
#pragma warning( disable : 4267 )

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#pragma warning( pop )

//https://github.com/labatrockwell/ofxWebsocketpp/blob/master/src/ofxWebsocketppClient.cpp
//https://github.com/wieden-kennedy/Cinder-WebSocketPP/blob/master/src/WebSocketClient.cpp

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>




typedef websocketpp::client<websocketpp::config::asio_tls_client> client;

// "c++ forward declaration" - declare classes names then classes first.
// functions that share types must be defined only after both classes finly defined.
// so you will see some function defined in the cpp file.

class websocket_endpoint; 


class connection_metadata : public std::enable_shared_from_this<connection_metadata> {
private:
    int m_id;
    websocketpp::connection_hdl m_hdl;
    std::string m_status;
    std::string m_uri;
    std::string m_server;
    std::string m_error_reason;
    std::vector<std::string> m_messages;
    client *m_endpoint;


public:
    client::connection_ptr con;
    
    typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;



    static void const empty_onopen(connection_metadata::ptr, client* c, websocketpp::connection_hdl hdl) {}
    std::function<void(connection_metadata::ptr self, client*, websocketpp::connection_hdl)> onopen= empty_onopen;

    static void const empty_onmessage(client::message_ptr msg) {}
    std::function<void( client::message_ptr)> onmessage= empty_onmessage;
  

    connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri, client * endpoint)
        : m_id(id)
        , m_hdl(hdl)
        , m_status("Connecting")
        , m_uri(uri)
        , m_server("N/A")
        , con(nullptr)
    {
        m_endpoint= endpoint;
    }

    connection_metadata::ptr set_onopen(std::function<void(connection_metadata::ptr self, client*, websocketpp::connection_hdl)> onopen_arg) {
        onopen = onopen_arg;
        return shared_from_this();
    }
    void on_open(client* c, websocketpp::connection_hdl hdl) {
        m_status = "Open";

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
        onopen(shared_from_this(), c, hdl);
    }

    void on_fail(client* c, websocketpp::connection_hdl hdl) {
        m_status = "Failed";

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
        m_error_reason = con->get_ec().message();
    }

    void on_close(client* c, websocketpp::connection_hdl hdl) {
        m_status = "Closed";
        client::connection_ptr con = c->get_con_from_hdl(hdl);
        std::stringstream s;
        s << "close code: " << con->get_remote_close_code() << " ("
            << websocketpp::close::status::get_string(con->get_remote_close_code())
            << "), close reason: " << con->get_remote_close_reason();
        m_error_reason = s.str();
    }

    connection_metadata::ptr set_onmessage(std::function<void(client::message_ptr)> onmessage_arg) {
        onmessage = onmessage_arg;
        return shared_from_this();
    }
    void on_message(websocketpp::connection_hdl, client::message_ptr msg) {
        
        if (msg->get_opcode() == websocketpp::frame::opcode::text) {
            m_messages.push_back("<< " + msg->get_payload());
        }
        else {
            m_messages.push_back("<< " + websocketpp::utility::to_hex(msg->get_payload()));
        }
        onmessage(msg);
    }

    websocketpp::connection_hdl get_hdl() const {
        return m_hdl;
    }

    int get_id() const {
        return m_id;
    }


    void send(std::string message);

    void send(std::string message, websocketpp::frame::opcode::value opcode);

    connection_metadata::ptr connect() {
        m_endpoint->connect(con);
        return shared_from_this();
    }

    void close();

    std::string get_status() const {
        return m_status;
    }

    void record_sent_message(std::string message) {
        m_messages.push_back(">> " + message);
    }

    friend std::ostream& operator<< (std::ostream& out, connection_metadata const& data);
};


class websocket_endpoint {
public:
    websocket_endpoint() : m_next_id(0) {
        m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
        m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

        //m_endpoint.set_access_channels(websocketpp::log::alevel::all);
        //m_endpoint.set_error_channels(websocketpp::log::elevel::all);

        m_endpoint.init_asio();
        m_endpoint.start_perpetual();

        m_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(&client::run, &m_endpoint);
    }

    ~websocket_endpoint() {
        m_endpoint.stop_perpetual();

        for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
            if (it->second->get_status() != "Open") {
                // Only close open connections
                continue;
            }

            std::cout << "> Closing connection " << it->second->get_id() << std::endl;

            websocketpp::lib::error_code ec;
            m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
            if (ec) {
                std::cout << "> Error closing connection " << it->second->get_id() << ": "
                    << ec.message() << std::endl;
            }
        }

        m_thread->join();
    }

    connection_metadata::ptr create(std::string const& uri);



    void close(int id, websocketpp::close::status::value code, std::string reason) {
        websocketpp::lib::error_code ec;

        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            std::cout << "> No connection found with id " << id << std::endl;
            return;
        }

        m_endpoint.close(metadata_it->second->get_hdl(), code, reason, ec);
        if (ec) {
            std::cout << "> Error initiating close: " << ec.message() << std::endl;
        }
        m_connection_list.erase(id);
    }

    void send(int id, std::string message) {
        websocketpp::lib::error_code ec;

        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            std::cout << "> No connection found with id " << id << std::endl;
            return;
        }

        m_endpoint.send(metadata_it->second->get_hdl(), message, websocketpp::frame::opcode::text, ec);
        if (ec) {
            std::cout << "> Error sending message: " << ec.message() << std::endl;
            return;
        }

        metadata_it->second->record_sent_message(message);
    }

    connection_metadata::ptr get_metadata(int id) const {
        con_list::const_iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            return connection_metadata::ptr();
        }
        else {
            return metadata_it->second;
        }
    }
private:
    typedef std::map<int, connection_metadata::ptr> con_list;

    client m_endpoint;

    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

    con_list m_connection_list;
    int m_next_id;
};




#endif // WEBSOCKETPP_TLS_UTIL_HPP