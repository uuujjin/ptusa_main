#ifndef modbus_client_h__
#define modbus_client_h__
#include "tcp_client.h"

class modbus_client 
	{
	protected:
		tcp_client* tcpclient;
		int modbus_expected_length;
		int modbus_async_result;
	public:
		modbus_client(unsigned int id, char* ip, unsigned int port = 502, unsigned long exchangetimeout = 50);
		int get_id();
		//���������� ������� ��������� modbus
		int read_discrete_inputs(unsigned int start_address, unsigned int quantity);
		int read_coils(unsigned int start_address, unsigned int quantity);
		int read_holding_registers(unsigned int address, unsigned int quantity);
		int read_input_registers(unsigned int address, unsigned int quantity);
		int write_coil(unsigned int address, unsigned char value);
		int force_multiply_coils(unsigned int address, unsigned int quantity);
		int write_multiply_registers(unsigned int address, unsigned int quantity);
		int async_read_discrete_inputs(unsigned int start_address, unsigned int quantity);
        int async_read_coils(unsigned int start_address, unsigned int quantity);
        int async_read_holding_registers(unsigned int address, unsigned int quantity);
        int async_read_input_registers(unsigned int address, unsigned int quantity);
        int async_write_coil(unsigned int address, unsigned char value);
        int async_force_multiply_coils(unsigned int address, unsigned int quantity);
        int async_write_multiply_registers(unsigned int address, unsigned int quantity);
		int get_async_result();
		//������� ��� ������ � ������� �� lua
		void zero_output_buff();
		void set_int2(unsigned int address, int_2 value);
		int_2 get_int2(unsigned int address);
		void set_int4(unsigned int address, int_4 value);
		int_4 get_int4(unsigned int address);
		void set_float(unsigned int address, float value);
		float get_float(unsigned int address);
		void set_bit(unsigned int address, int value);
		int get_bit(unsigned int address);
		~modbus_client();
	};
#endif // modbus_client_h__
