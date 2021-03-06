#include "WorkerSession.hpp"

namespace alchemist {

// =============================================================================================
//                                         WorkerSession
// =============================================================================================

WorkerSession::WorkerSession(tcp::socket _socket, GroupWorker & _group_worker) :
		Session(std::move(_socket)), group_worker(_group_worker) { }

WorkerSession::WorkerSession(tcp::socket _socket, GroupWorker & _group_worker, SessionID ID, ClientID _clientID) :
		Session(std::move(_socket), ID, _clientID), group_worker(_group_worker) { }

WorkerSession::WorkerSession(tcp::socket _socket, GroupWorker & _group_worker, SessionID ID, ClientID _clientID, Log_ptr & _log) :
		Session(std::move(_socket), ID, _clientID, _log), group_worker(_group_worker) { }

void WorkerSession::start()
{
	log->info("{} Connection established", session_preamble());
//	worker.write_session(shared_from_this());
	read_header();
}

void WorkerSession::remove_session()
{
	log->info("{} Removing session", session_preamble());
//	worker.remove_session();
}

int WorkerSession::handle_message()
{
//	log->info("{}", read_msg.to_string());

	client_command command = read_msg.cc;

	if (command == HANDSHAKE) {
		handle_handshake();
		read_header();
	}
	else {
		ClientID _clientID = read_msg.clientID;
		SessionID _sessionID = read_msg.sessionID;

		if (sessionID != _sessionID) {
			log->info("{} Error in WorkerSession: Wrong session ID", session_preamble());
		}

		switch (command) {
			case REQUEST_TEST_STRING:
				send_test_string();
				read_header();
				break;
			case SEND_TEST_STRING:
				send_response_string();
				read_header();
				break;
			case SEND_MATRIX_BLOCKS:
				receive_matrix_blocks();
				read_header();
				break;
			case REQUEST_MATRIX_BLOCKS:
				send_matrix_blocks();
				read_header();
				break;
		}
	}

	return 0;
}

bool WorkerSession::send_matrix_blocks()
{
	uint32_t num_blocks = 0;
	uint64_t i, j;
	double temp;
	DoubleArrayBlock_ptr in_block, out_block;

	ArrayID matrixID = read_msg.read_uint16();

	log->info("{} Sending data blocks for array {}", session_preamble(), matrixID);

	clock_t start = clock();

	write_msg.start(clientID, sessionID, REQUEST_MATRIX_BLOCKS);
	write_msg.write_uint16(matrixID);

	while (!read_msg.eom()) {

		in_block = read_msg.read_DoubleArrayBlock();
		out_block = std::make_shared<ArrayBlock<double>>(*in_block);

		write_msg.write_DoubleArrayBlock(out_block);

		for (i = in_block->dims[0][0]; i < in_block->dims[1][0]; i += in_block->dims[2][0])
			for (j = in_block->dims[0][1]; j < in_block->dims[1][1]; j += in_block->dims[2][1]) {
				group_worker.get_value(matrixID, i, j, temp);
				out_block->write_next(&temp);
			}

		num_blocks++;
	}

	clock_t end = clock();
	log->info("{} Sending data blocks took {}ms", session_preamble(), 1000.0*((double) (end - start))/((double) CLOCKS_PER_SEC));
	flush();

	return true;
}

bool WorkerSession::receive_matrix_blocks()
{
	uint32_t num_blocks = 0;
	uint64_t i, j;
	double temp;

	ArrayID matrixID = read_msg.read_ArrayID();

	log->info("{} Receiving data blocks for array {}", session_preamble(), matrixID);

	clock_t start = clock();

	while (!read_msg.eom()) {

		DoubleArrayBlock_ptr block = read_msg.read_DoubleArrayBlock();

		for (i = block->dims[0][0]; i < block->dims[1][0]; i += block->dims[2][0])
			for (j = block->dims[0][1]; j < block->dims[1][1]; j += block->dims[2][1]) {
				block->read_next(&temp);
				group_worker.set_value(matrixID, i, j, temp);
			}

		num_blocks++;
//		log->info("{} Array {}: Received matrix block (rows {}-{}, columns {}-{})", session_preamble(), matrixID, row_start, row_end, col_start, col_end);
	}

	write_msg.start(clientID, sessionID, SEND_MATRIX_BLOCKS);
	write_msg.write_uint16(matrixID);
	write_msg.write_uint32(num_blocks);

	clock_t end = clock();
	log->info("{} Receiving data blocks took {}ms", session_preamble(), 1000.0*((double) (end - start))/((double) CLOCKS_PER_SEC));
	flush();

	return true;
}

bool WorkerSession::send_test_string()
{
//	char buffer[4];
//	std::stringstream test_str;
//
//	sprintf(buffer, "%03d", worker.getID());
//	test_str << "This is a test string from Alchemist worker " << buffer;
//
//	write_msg.start(clientID, ID, REQUEST_TEST_STRING);
//	write_msg.write_string(test_str.str());
//	flush();

	return true;
}


//int WorkerSession::receive_test_string(const char * data, const uint32_t length)
//{
//	log->info("{} Test string: {}", session_preamble(), string(data, length));
//
//	return 0;
//}
//
//string WorkerSession::get_test_string()
//{
//	std::stringstream ss;
//	ss << "This is a test string from Alchemist worker " << ID;
//
//	return ss.str();
//}


bool WorkerSession::send_response_string()
{
	string test_str = "Alchemist worker received: '";
	test_str += read_msg.read_string();
	test_str += "'";

	write_msg.start(clientID, sessionID, SEND_TEST_STRING);
	write_msg.write_string(test_str);
	flush();

	return true;
}


}			// namespace alchemist
