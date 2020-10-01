/*
 * Copyright (c) 2014, Peter Thorson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the WebSocket++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PETER THORSON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "tls_util.hpp"


int main(int argc, char* argv[]) {

    bool done = false;
    std::string input;
    websocket_endpoint endpoint; // thread
    connection_metadata::ptr last_connection;
    while (!done) {
        std::cout << "Enter Command: ";
        std::getline(std::cin, input);

        if (input == "quit") {
            done = true;
        }
        else if (input == "help") {
            std::cout
                << "\nCommand List:\n"
                << "connect <ws uri>\n"
                << "send <connection id> <message>\n"
                << "close <connection id> [<close code:default=1000>] [<close reason>]\n"
                << "show <connection id>\n"
                << "help: Display this help text\n"
                << "quit: Exit the program\n"
                << std::endl;
        }
        else if (input.substr(0, 7) == "connect") {
            int id = endpoint.create(input.substr(8))->connect()->get_id();
            if (id != -1) {
                std::cout << "> Created connection with id " << id << std::endl;
            }
        }
        else if (input.substr(0, 4) == "send") {
            std::stringstream ss(input);

            std::string cmd;
            int id;
            std::string message;

            ss >> cmd >> id;
            std::getline(ss, message);

            endpoint.send(id, message);
        }
        else if (input.substr(0, 5) == "close") {
            std::stringstream ss(input);

            std::string cmd;
            int id;
            int close_code = websocketpp::close::status::normal;
            std::string reason;

            ss >> cmd >> id >> close_code;
            std::getline(ss, reason);

            endpoint.close(id, close_code, reason);
        }
        else if (input.substr(0, 4) == "show") {
            int id = atoi(input.substr(5).c_str());

            connection_metadata::ptr metadata = endpoint.get_metadata(id);
            if (metadata) {
                std::cout << *metadata << std::endl;
            }
            else {
                std::cout << "> Unknown connection id " << id << std::endl;
            }
        }
        else if (input.substr(0, 4) == "test") {
            last_connection =
                endpoint.create("wss://echo.websocket.org")
                ->set_onopen([](connection_metadata::ptr self, client* c, websocketpp::connection_hdl) {
                self->send("hello");

                std::cout << std::endl << ">> hello" << std::endl;
                    })
                ->set_onmessage([](client::message_ptr msg) {
                        if (msg->get_opcode() == websocketpp::frame::opcode::text) {
                            std::cout << std::endl << "<< " + msg->get_payload() << std::endl;
                        }

                    })->connect();
        }
        else if (input.substr(0, 5) == "lsend") {
            std::cout << std::endl << ">>" << input.substr(5) << std::endl;
            last_connection->send(input.substr(5));
        }
        else {
            std::cout << "> Unrecognized Command" << std::endl;
        }
    }

    return 0;

}

