
#include <string>

#include "database.h"
#include "databasemysql.h"

boost::recursive_mutex DBQuery::databaseLock;
Database* _Database::_instance = NULL;

Database* _Database::getInstance()
{
    if(!_instance)
    {
        _instance = new DatabaseMySQL;
        //_instance = new Database;
    }

    _instance->use();
    return _instance;
}

DBResult* _Database::verifyResult(DBResult* result)
{
    if(result->next())
        return result;

    result->free();
    result = NULL;
    return NULL;
}

DBInsert::DBInsert(Database* db)
{
    m_db = db;
    m_rows = 0;
    // checks if current database engine supports multiline INSERTs
    m_multiLine = m_db->getParam(DBPARAM_MULTIINSERT);
}

void DBInsert::setQuery(const std::string& query)
{
    m_query = query;
    m_buf = "";
    m_rows = 0;
}

bool DBInsert::addRow(const std::string& row)
{
    if(!m_multiLine) // executes INSERT for current row
        return m_db->executeQuery(m_query + "(" + row + ")");

    m_rows++;
    int32_t size = m_buf.length();
    // adds new row to buffer
    if(!size)
        m_buf = "(" + row + ")";
    else if(size > 8192)
    {
        if(!execute())
            return false;

        m_buf = "(" + row + ")";
    }
    else
        m_buf += ",(" + row + ")";

    return true;
}

bool DBInsert::addRow(std::stringstream& row)
{
    bool ret = addRow(row.str());
    row.str("");
    return ret;
}

bool DBInsert::execute()
{
    if(!m_multiLine || m_buf.length() < 1) // INSERTs were executed on-fly
        return true;

    if(!m_rows) //no rows to execute
        return true;

    m_rows = 0;
    // executes buffer
    bool res = m_db->executeQuery(m_query + m_buf);
    m_buf = "";
    return res;
}
