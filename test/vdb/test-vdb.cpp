/*===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*/

/**
* Unit tests for C++ wrapper for the VDB API
*/

#include "vdb.hpp"

#include <ktst/unit_test.hpp>

#include <sstream>

using namespace std;
using namespace VDB;

TEST_SUITE(VdbTestSuite);

TEST_CASE(Manager_Construction)
{
    Manager m;
}

TEST_CASE(Manager_Schema_Bad)
{
    const string Text = "this is a bad schema";
    REQUIRE_THROW( Manager().schema(Text.size(), Text.c_str()) );
}

TEST_CASE(Manager_Schema_Good)
{
    const string Text = "version 1;";
    Schema s = Manager().schema(Text.size(), Text.c_str());
    ostringstream out;
    out << s;
    REQUIRE_EQ( string("version 1;\n"), out.str() );
}

TEST_CASE(Manager_Schema_Include)
{
    const string Text = "version 1; include 'inc.vschema';";
    const string IncludePath = "./data";
    Schema s = Manager().schema(Text.size(), Text.c_str(), IncludePath.c_str());

    ostringstream out;
    out << s;
    REQUIRE_EQ( string("version 1;\ntypedef U32 INSDC:SRA:spotid_t; /* size 32 */\n"), out.str() );
}

TEST_CASE(Manager_SchemaFromFile_Bad)
{
    REQUIRE_THROW( Manager().schemaFromFile("bad file") );
}

TEST_CASE(Manager_SchemaFromFile)
{
    Schema s = Manager().schemaFromFile("./data/inc.vschema");
    ostringstream out;
    out << s;
    REQUIRE_EQ( string("version 1;\ntypedef U32 INSDC:SRA:spotid_t; /* size 32 */\n"), out.str() );
}

TEST_CASE(Manager_Database_Bad)
{
    REQUIRE_THROW( Manager()["./data/database"] );
}

const string DatabasePath = "./data/SRR6336806";
const string TablePath = "./data/ERR3487613";

TEST_CASE(Manager_Database_Good)
{
    Database d = Manager()[DatabasePath];
}
TEST_CASE(Manager_OpenDatabase_Good)
{
    Database d = Manager().openDatabase(DatabasePath);
}

TEST_CASE(Manager_OpenTable_Bad)
{   // throws if given a database
    REQUIRE_THROW( Manager().openTable(DatabasePath) );
}
TEST_CASE(Manager_OpenTable_Good)
{
    Table t = Manager().openTable(TablePath);
}

TEST_CASE(Database_Table_Bad)
{
    Database d = Manager()[DatabasePath];
    REQUIRE_THROW( d["NOSUCHTABLE"]);
}

TEST_CASE(Database_Table_Good)
{
    Database d = Manager()[DatabasePath];
    Table t = d["SEQUENCE"];
}

TEST_CASE(Table_ReadCursor1_BadColumn)
{
    Table t = Manager()[DatabasePath]["SEQUENCE"];
    const char * cols[] = {"a", "b"};
    REQUIRE_THROW( t.read( 2, cols ) );
}

TEST_CASE(Table_ReadCursor1)
{
    Table t = Manager()[DatabasePath]["SEQUENCE"];
    const char * cols[] = {"READ", "NAME"};
    Cursor c = t.read( 2, cols );
}

TEST_CASE(Table_ReadCursor2_BadColumn)
{
    Table t = Manager()[DatabasePath]["SEQUENCE"];
    REQUIRE_THROW( t.read( {"a", "b"} ) );
}

TEST_CASE(Table_ReadCursor2)
{
    Table t = Manager()[DatabasePath]["SEQUENCE"];
    Cursor c = t.read( {"READ", "NAME"} );
}

TEST_CASE(Cursor_Columns)
{
    Table t = Manager()[DatabasePath]["SEQUENCE"];
    Cursor c = t.read( {"READ", "NAME"} );
    REQUIRE_EQ( 2u, c.columns() );
}

TEST_CASE(Cursor_RowRange)
{
    Table t = Manager()[DatabasePath]["SEQUENCE"];
    Cursor c = t.read( {"READ", "NAME"} );
    auto r = c.rowRange();
    REQUIRE_EQ( Cursor::RowID(1), r.first );
    REQUIRE_EQ( Cursor::RowID(2608), r.second ); // exclusive
}

TEST_CASE(Cursor_ReadOne)
{
    Table t = Manager()[DatabasePath]["SEQUENCE"];
    Cursor c = t.read( {"READ", "NAME"} );
    Cursor::RawData rd = c.read( 1, 2 );
    REQUIRE_EQ( size_t(1), rd.size() );
    REQUIRE_EQ( string("1"), rd.asString() );
}

TEST_CASE(Cursor_ReadN)
{
    Table t = Manager()[DatabasePath]["SEQUENCE"];
    Cursor c = t.read( {"READ", "NAME"} );
    Cursor::RawData rd[2];
    c.read( 2, 2, rd );
    REQUIRE_EQ( string("AAGTCG"), rd[0].asString().substr(0, 6) );
    REQUIRE_EQ( string("2"), rd[1].asString() );
}

TEST_CASE(Cursor_ForEach)
{
    Table t = Manager()[DatabasePath]["SEQUENCE"];
    Cursor c = t.read( {"READ", "NAME"} );
    auto check = [&](Cursor::RowID row, const vector<Cursor::RawData>& values )
    {
        REQUIRE_LT( (size_t)0, values[0].asString().size() );
        ostringstream rowId;
        rowId << row;
        REQUIRE_EQ( rowId.str(), values[1].asString() );
    };
    uint64_t n = c.foreach( check );
    REQUIRE_EQ( (uint64_t)2607, n );
}

TEST_CASE(Cursor_ForEachWithFilter)
{
    Table t = Manager()[DatabasePath]["SEQUENCE"];
    Cursor c = t.read( {"READ", "NAME"} );
    auto check = [&](Cursor::RowID row, bool keep, const vector<Cursor::RawData>& values )
    {
        if ( keep )
        {
            REQUIRE_EQ( 1, (int)(row % 2) );
            REQUIRE_EQ( size_t(2), values.size() );
        }
        else
        {
            REQUIRE_EQ( 0, (int)(row % 2) );
            REQUIRE_EQ( size_t(0), values.size() );
        }
    };
    auto filter = [&]( const Cursor & cur, Cursor::RowID row ) -> bool { return bool(row % 2); };
    uint64_t n = c.foreach( filter, check );
    REQUIRE_EQ( (uint64_t)2607, n );
}

TEST_CASE( RawData_asVector_badCast )
{
    Table t = Manager()[DatabasePath]["SEQUENCE"];
    Cursor c = t.read( {"READ_START", "NAME"} );
    Cursor::RawData rd = c.read( 1, 1 );
    REQUIRE_THROW( rd.asVector<uint16_t>() );
}

TEST_CASE( RawData_asVector )
{
    Table t = Manager()[DatabasePath]["SEQUENCE"];
    Cursor c = t.read( {"READ_START", "NAME"} );
    Cursor::RawData rd = c.read( 1, 1 );
    auto cv = rd.asVector<uint32_t>();
    REQUIRE_EQ( size_t(2), cv.size() );
    REQUIRE_EQ( uint32_t(0), cv[0] );
    REQUIRE_EQ( uint32_t(301), cv[1] );
}

TEST_CASE( RawData_value_badCast )
{
    Table t = Manager()[DatabasePath]["SEQUENCE"];
    Cursor c = t.read( {"SPOT_LEN", "NAME"} );
    Cursor::RawData rd = c.read( 1, 1 );
    REQUIRE_THROW( rd.value<uint16_t>() );
}

TEST_CASE( RawData_value )
{
    Table t = Manager()[DatabasePath]["SEQUENCE"];
    Cursor c = t.read( {"SPOT_LEN", "NAME"} );
    Cursor::RawData rd = c.read( 1, 1 );
    auto v = rd.value<uint32_t>();
    REQUIRE_EQ( uint32_t(602), v );
}

int main (int argc, char *argv [])
{
    return VdbTestSuite(argc, argv);
}
