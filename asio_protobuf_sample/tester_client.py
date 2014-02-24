#!/usr/bin/python
#
# tester_client.py: simple testing client for the server. Suitable for
# usage from the python interactive prompt.
#
# Eli Bendersky (eliben@gmail.com)
# This code is in the public domain
#

from __future__ import print_function

import sys
from socket import *
import struct
from stringdb_pb2 import *


def make_socket(port=4050):
    """ Create a socket on localhost and return it.
    """
    sockobj = socket(AF_INET, SOCK_STREAM)
    sockobj.connect(('192.168.1.103', port))
    return sockobj


def send_message(sock, message):
    """ Send a serialized message (protobuf Message interface)
        to a socket, prepended by its length packed in 4 bytes.
    """
    s = message.SerializeToString()
    #print("string len: %d" %len(s))
    packed_len = struct.pack('>L', len(s))
    packed_message = packed_len + s
    sock.send(packed_message)


def socket_read_n(sock, n):
    """ Read exactly n bytes from the socket.
        Raise RuntimeError if the connection closed before n bytes were read.
    """
    buf = ''
    while n > 0:
        data = sock.recv(n)
        if data == '':
            raise RuntimeError('unexpected connection close')
        buf += data
        n -= len(data)
    return buf
    

def get_response(sock):
    """ Read a serialized response message from a socket.
    """
    msg = Response()
    len_buf = socket_read_n(sock, 4)
    msg_len = struct.unpack('>L', len_buf)[0]
    msg_buf = socket_read_n(sock, msg_len)
    msg.ParseFromString(msg_buf)
    return msg

def send_read_request(sock):
	rq = Request()
	rq.type = READ
	print("Send reqeust: %d" %rq.type)
	rq.request_read.request_read_data.application_id = 9	
	print("application_id: %d" %rq.request_read.request_read_data.application_id)
	rq.request_read.request_read_data.attribute_id.append(1)
	print("attribute_id 1: %d" %rq.request_read.request_read_data.attribute_id[0])
	rq.request_read.request_read_data.attribute_id.append(24)
	print("attribute_id 2: %d" %rq.request_read.request_read_data.attribute_id[1])
	rq.request_read.request_read_data.attribute_id.append(128)
	print("attribute_id 2: %d" %rq.request_read.request_read_data.attribute_id[2])
	print("")
	send_message(sock, rq)
	return get_read_response(sock)
	
def get_read_response(sock):
	""" Read a serialized response message from a socket.
	"""
	
	msg = Response()
	len_buf = socket_read_n(sock, 4)
	msg_len = struct.unpack('>L', len_buf)[0]
	msg_buf = socket_read_n(sock, msg_len)
	msg.ParseFromString(msg_buf)
	print("Get response: %d" %msg.type)
	print("application_id: %d" %msg.response_read.response_read_data.application_id)
	attrDataLen = len(msg.response_read.response_read_data.attribute_data)
	print("attribute id size: %d" %attrDataLen)
	for index in range(attrDataLen):
		print("attribute id: %d, attribute value: %s" %(msg.response_read.response_read_data.attribute_data[index].attribute_id, msg.response_read.response_read_data.attribute_data[index].attribute_value))
	print("")
	return msg

def send_write_request(sock):
	rq = Request()
	rq.type = WRITE
	print("Send reqeust: %d" %rq.type)
	rq.request_write.request_write_data.application_id = 8	
	print("application_id: %d" %rq.request_write.request_write_data.application_id)
	
	attribute_data1 = rq.request_write.request_write_data.attribute_data.add()
	attribute_data1.attribute_id = 1
	attribute_data1.attribute_value = "attribute 1"
	
	attribute_data1 = rq.request_write.request_write_data.attribute_data.add()
	attribute_data1.attribute_id = 16
	attribute_data1.attribute_value = "attribute 16"
	
	attribute_data1 = rq.request_write.request_write_data.attribute_data.add()
	attribute_data1.attribute_id = 128
	attribute_data1.attribute_value = "attribute 128"
	
	attrDataLen = len(rq.request_write.request_write_data.attribute_data)
	for index in range(attrDataLen):
		print("attribute id: %d, attribute value: %s" %(rq.request_write.request_write_data.attribute_data[index].attribute_id, rq.request_write.request_write_data.attribute_data[index].attribute_value))
	print("")
	send_message(sock, rq)
	return get_write_response(sock)
	
def get_write_response(sock):
	""" Read a serialized response message from a socket.
	"""
	
	msg = Response()
	len_buf = socket_read_n(sock, 4)
	msg_len = struct.unpack('>L', len_buf)[0]
	msg_buf = socket_read_n(sock, msg_len)
	msg.ParseFromString(msg_buf)
	print("Get response: %d" %msg.type)
	print("application_id: %d" %msg.response_write.response_write_data.application_id)
	writeDataLen = len(msg.response_write.response_write_data.write_data)
	print("attribute id size: %d" %writeDataLen)
	for index in range(writeDataLen):
		print("attribute id: %d, error number: %d" %(msg.response_write.response_write_data.write_data[index].attribute_id, msg.response_write.response_write_data.write_data[index].error_number))
	print("")
	return msg
	
if __name__ == '__main__':
	port = 4050
	if len(sys.argv) >= 2:
		port = int(sys.argv[1])

	sockobj = make_socket(port)
	send_read_request(sockobj)
	send_write_request(sockobj)

	
	

