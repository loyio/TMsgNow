#pragma once

#include "net_common.hpp"
#include "net_tsqueue.hpp"
#include "net_message.hpp"
#include "net_connection.hpp"

 namespace tplayn
{
    namespace net
    {
         template <typename T>
        class server_interface{
        public:
            server_interface(uint16_t port) 
                : m_asioAcceptor(m_ioc, boost::asio::ip::tcp::v4(), port){
            }

            virtual ~server_interface() { Stop(); }
            
            bool Start(){
                try{
                    WaitForClientConnection();
                    m_threadContext = std::thread([this]() { m_ioc.run(); });
                }
                catch (std::exception& e){
                    std::cerr << "[SERVER] Exception: " << e.what() << std::endl;
                    return false;
                }

                std::cout << "[SERVER] Started! \n";
                return true;
            }

            bool Stop(){
                m_ioc.stop();

                // Clean the thread
                if (m_threadContext.joinable()) m_threadContext.join();

                std::cout << "[SERVER] Stopped! \n";
                 
            }

            // async to wait for client connection
            void WaitForClientConnection(){
                m_asioAcceptor.async_accept([this](std::error_code ec, boost::asio::ip::tcp::socket socket){
                    if(!ec){
                        std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << std::endl;
                        std::shared_ptr<connection<T>> newconn = std::make_shared<connection<T>>(connection<T>::owner::server, m_ioc, std::move(socket), m_qMessageIn);


                        // Give the user server a chance to deny connection
                        if(onClientConnect(newconn)){
                            // connection allow, add to container
                            m_deqConnections.push_back(std::move(newconn));

                            m_deqConnections.back()->ConnectToClient(nIDCounter++);

                            std::cout << "[" << m_deqConnections.back()->GetID() << "] Connection Approved" << std::endl;
                        }else{
                        }

                    }else{
                        // Error has occured during acceptance
                        std::cout << "[SERVER] New Connection Error: " << ec.message() << std::endl;

                    }           

                    //wait for another connection
                    WaitForClientConnection();
                });
            }

            // sent message to specific client
            void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg){
                if(client && client->IsConnected()){
                    client->Send(msg);
                }else{
                    onClientDisconnect(client);
                    client.reset();
                    m_deqConnections.erase(std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end());
                }

            }

            // Send message to all client except the ignore client
            void MessageAllClient(const message<T>& msg, std::shared_ptr<connection<T>> pIgnoreClient = nullptr){

                bool bInvalidClientExists = false;

                for(auto& client : m_deqConnections){
                    if(client && client->IsConnected()){
                        if(client != pIgnoreClient){
                            client->Send(msg);
                        }
                    }else{
                        // can't be contacted, assume it has disconnected
                        onClientDisconnect(client);
                        client.rese();
                        bInvalidClientExists = true;
                    }
                }

                if(bInvalidClientExists){
                    m_deqConnections.erase(std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end());
                }
            }


            void Update(size_t nMaxMessage = -1){
                size_t nMessageCount = 0;
                while(nMessageCount < nMaxMessage && !m_qMessageIn.empty()){
                    // get the front message
                    auto msg = m_qMessageIn.pop_front();

                    // Pass to message handler
                    OnMessage(msg.remote, msg.msg);

                    nMessageCount++;
                }
            }

            
        protected:
            // when a client connects, you reject it by false
            virtual bool onClientConnect(std::shared_ptr<connection<T>> client){
                return false;
            }

            // when a client appears to have disconnected
            virtual void onClientDisconnect(std::shared_ptr<connection<T>> client){
                
            }

            // called when a message arrived
            virtual void OnMessage(std::shared_ptr<connection<T>> client, message<T>& msg){
            }

        protected:
            // Thread safe Queue for incomming message packets
            tsqueue<owned_message<T>> m_qMessageIn;

            // active validated connections
            std::deque<std::shared_ptr<connection<T>>> m_deqConnections;


            boost::asio::io_context m_ioc;
            std::thread m_threadContext;

            // need a asio context
            boost::asio::ip::tcp::acceptor m_asioAcceptor;

            // client id
            uint32_t nIDCounter = 10000;

        };


    }
}
