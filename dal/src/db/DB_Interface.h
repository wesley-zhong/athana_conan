#pragma once

class DBResult;
class DB_Interface
{
public:
	DB_Interface(const char * ip, unsigned int port);
	virtual ~DB_Interface();

public:
	virtual bool connect() = 0;
	virtual bool detach() = 0;
	virtual int execute(DBResult * result, const char * cmd, int len = 0) = 0;
	virtual const char * getError() = 0;
	virtual int getErrno() = 0;
	virtual bool ping() = 0;
protected:
	std::string m_ip;
	unsigned int m_port;
};