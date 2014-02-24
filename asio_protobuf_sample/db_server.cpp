//
// db_server.cpp: DbServer implementation
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//
#include "db_server.h"
#include "packedmessage.h"
#include "stringdb.pb.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <boost/enable_shared_from_this.hpp>

using namespace std;
namespace asio = boost::asio;
using asio::ip::tcp;
using boost::uint8_t;


#define DEBUG false


// Database connection - handles a connection with a single client.
// Create only through the DbConnection::create factory.
//
class DbConnection : public boost::enable_shared_from_this<DbConnection>
{
public:
    typedef boost::shared_ptr<DbConnection> Pointer;
    typedef boost::shared_ptr<stringdb::Request> RequestPointer;
    typedef boost::shared_ptr<stringdb::Response> ResponsePointer;

    static Pointer create(asio::io_service& io_service)
    {
        return Pointer(new DbConnection(io_service));
    }

    tcp::socket& get_socket()
    {
        return m_socket;
    }

    void start()
    {
        start_read_header();
    }

private:
    tcp::socket m_socket;
    vector<uint8_t> m_readbuf;
    PackedMessage<stringdb::Request> m_packed_request;

    DbConnection(asio::io_service& io_service)
        : m_socket(io_service)
        , m_packed_request(boost::shared_ptr<stringdb::Request>(new stringdb::Request()))
    {
    }
    
    void handle_read_header(const boost::system::error_code& error)
    {
        DEBUG && (cerr << "handle read " << error.message() << '\n');
        if (!error) {
            DEBUG && (cerr << "Got header!\n");
            DEBUG && (cerr << show_hex(m_readbuf) << endl);
            unsigned msg_len = m_packed_request.decode_header(m_readbuf);
            DEBUG && (cerr << "meg_len: " << msg_len << " bytes\n");
            start_read_body(msg_len);
        }
    }

    void handle_read_body(const boost::system::error_code& error)
    {
        DEBUG && (cerr << "handle body " << error << '\n');
        if (!error) {
            DEBUG && (cerr << "Got body!\n");
            DEBUG && (cerr << show_hex(m_readbuf) << endl);
            handle_request();
            start_read_header();
        }
    }

    // Called when enough data was read into m_readbuf for a complete request
    // message. 
    // Parse the request, execute it and send back a response.
    //
    void handle_request()
    {
        if (m_packed_request.unpack(m_readbuf)) {
            RequestPointer req = m_packed_request.get_msg();
			
			ResponsePointer resp = prepare_response(req);
            vector<uint8_t> writebuf;
            PackedMessage<stringdb::Response> resp_msg(resp);
            resp_msg.pack(writebuf);
			for(int i = 0; i < writebuf.size(); i++)
				DEBUG && printf("response buffer: %d, %d\n", writebuf[i], writebuf.size());
            asio::write(m_socket, asio::buffer(writebuf));
        }
    }

    void start_read_header()
    {
        m_readbuf.resize(HEADER_SIZE);
        asio::async_read(m_socket, asio::buffer(m_readbuf),
                boost::bind(&DbConnection::handle_read_header, shared_from_this(),
                    asio::placeholders::error));
    }

    void start_read_body(unsigned msg_len)
    {
        // m_readbuf already contains the header in its first HEADER_SIZE
        // bytes. Expand it to fit in the body as well, and start async
        // read into the body.
        //
        m_readbuf.resize(HEADER_SIZE + msg_len);
        asio::mutable_buffers_1 buf = asio::buffer(&m_readbuf[HEADER_SIZE], msg_len);
        asio::async_read(m_socket, buf,
                boost::bind(&DbConnection::handle_read_body, shared_from_this(),
                    asio::placeholders::error));
    }

	ResponsePointer prepare_response(RequestPointer req)
	{
		if(req->type() == stringdb::READ)
		{
			DEBUG && printf("req->request_read().ByteSize() %d\n", req->request_read().ByteSize());
			int appID = req->request_read().request_read_data().application_id();
			int attrSize = req->request_read().request_read_data().attribute_id_size();
				
			ResponsePointer resp(new stringdb::Response);
			//set type
			resp->set_type(stringdb::READ);
			stringdb::Response_ResponseRead* pResponseRead = resp->mutable_response_read();

			stringdb::Response_ResponseRead_ResponseReadData* pResponseReadData = pResponseRead->mutable_response_read_data();
			pResponseReadData->set_application_id(appID);
			
			for (int i = 0; i < attrSize; i++)
			{
				stringdb::Response_ResponseRead_ResponseReadData_AttributeData* pData = pResponseReadData->add_attribute_data();
				pData->set_attribute_id(req->request_read().request_read_data().attribute_id(i));

				string strValue;
				stringstream ssValue;
				ssValue << i;
				strValue = ssValue.str();
				pData->set_attribute_value("ok " + strValue);
			}
			return resp;
		}
		else if(req->type() == stringdb::WRITE)
		{
			int appID = req->request_write().request_write_data().application_id();
			int attrDataSize = req->request_write().request_write_data().attribute_data_size();
			ResponsePointer resp(new stringdb::Response);
			resp->set_type(stringdb::WRITE);
			stringdb::Response_ResponseWrite* pResponseWrite = resp->mutable_response_write();
			stringdb::Response_ResponseWrite_ResponseWriteData* pResponseWriteData = pResponseWrite->mutable_response_write_data();
			pResponseWriteData->set_application_id(appID);
			
			for (int i = 0; i < attrDataSize; i++)
			{
				stringdb::Response_ResponseWrite_ResponseWriteData_WriteData* pData = pResponseWriteData->add_write_data();
				stringdb::Request_RequestWrite_RequestWriteData_AttributeData* pReqAttrData = &req->request_write().request_write_data().attribute_data(i);
				pData->set_attribute_id(pReqAttrData->attribute_id());
				pData->set_error_number(i);
			}
			return resp;
		}
	
	};
};


struct DbServer::DbServerImpl
{
    tcp::acceptor acceptor;

    DbServerImpl(asio::io_service& io_service, unsigned port)
        : acceptor(io_service, tcp::endpoint(tcp::v4(), port))
    {
        start_accept();
    }

    void start_accept()
    {
        // Create a new connection to handle a client. Passing a reference
        // to db to each connection poses no problem since the server is 
        // single-threaded.
        //
        DbConnection::Pointer new_connection = 
            DbConnection::create(acceptor.get_io_service());

        // Asynchronously wait to accept a new client
        //
        acceptor.async_accept(new_connection->get_socket(),
            boost::bind(&DbServerImpl::handle_accept, this, new_connection,
                asio::placeholders::error));
    }

    void handle_accept(DbConnection::Pointer connection,
            const boost::system::error_code& error)
    {
        // A new client has connected
        //
        if (!error) {
            // Start the connection
            //
            connection->start();

            // Accept another client
            //
            start_accept();
        }
    }
};


DbServer::DbServer(asio::io_service& io_service, unsigned port)
    : d(new DbServerImpl(io_service, port))
{
}


DbServer::~DbServer()
{
}


