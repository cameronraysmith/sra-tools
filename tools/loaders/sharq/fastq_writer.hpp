#ifndef __FASTQ_WRITER_HPP__
#define __FASTQ_WRITER_HPP__

/**
 * @file fastq_writer.hpp
 * @brief FASTQ Writer
 *
 */

/*
* ===========================================================================
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
* Author:  Andrei Shkeda
*
* File Description: FASTQ Writer
*
* ===========================================================================
*/


#include "fastq_read.hpp"
#include "fastq_error.hpp"
#include "sra-tools/writer.hpp"
#include <spdlog/spdlog.h>
#include "spdlog/sinks/base_sink.h"
#include <spdlog/sinks/stdout_sinks.h>
#include <insdc/sra.h>
#include <json.hpp>
#include <fstream>

#define LOCALDEBUG

/**
 * @brief Generic writer with std output
 *
 */
class generic_writer
{
public:
    void set_platform(uint8_t platform) {}
    void write_spot(const vector<CFastqRead>& reads)
    {
        if (reads.empty())
            return;
        const auto& first_read = reads.front();
        cout << "Spot: " << first_read.Spot() << "\nreads:";
        for (const auto& read : reads) {
            //auto sz = read.Sequence().size();
            cout << " " << read.ReadNum() << "(" << (read.Type() == 0 ? "T" : "B") << ")" << "\n";
            cout << read.Sequence() << "\n";
            cout << "+\n";
            cout << read.Quality() << endl;
        }
        //cout << endl;
    }
};

/**
 * @brief Redirects spdlog logging messages to general-loader
 *
 * @tparam Mutex
 */
template<typename Mutex>
class general_writer_sink : public spdlog::sinks::base_sink<Mutex>
{
public:
    general_writer_sink(shared_ptr<Writer2>& writer_)
        : //spdlog::sinks::base_sink<Mutex>(logger_name)
        writer(writer_)
    {}
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        if (msg.level >= SPDLOG_LEVEL_ERROR)
            writer->errorMessage(fmt::to_string(formatted));
        else
            writer->logMessage(fmt::to_string(formatted));
        //writer->progressMessage(fmt::to_string(formatted));
    }
    void flush_() override {}
    shared_ptr<Writer2> writer;
};

using general_writer_sink_mt = general_writer_sink<std::mutex>;

/**
 * @brief Create MT logger with general-loader sinker
 *
 * @tparam Factory
 * @param logger_name
 * @param writer
 * @return std::shared_ptr<spdlog::logger>
 */
template<typename Factory = spdlog::synchronous_factory>
inline std::shared_ptr<spdlog::logger> general_writer_logger_mt(const std::string &logger_name, shared_ptr<Writer2>& writer)
{
    // use this one to create a logger that sinks to general-loader only
    //auto gw_logger = Factory::template create<general_writer_sink_mt>(logger_name, writer);

    // this logger uses two sinke: stderr and general-loader
    // each sinker has its own formatting pattern
    auto stderr_sinker = make_shared<spdlog::sinks::stderr_sink_mt>();
    stderr_sinker->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    auto gl_sinker = make_shared<general_writer_sink_mt>(writer);
    gl_sinker->set_pattern("%v");

    auto gw_logger = make_shared<spdlog::logger>("multi_sink", spdlog::sinks_init_list({
        stderr_sinker,
        gl_sinker
    }));

    return gw_logger;
}


/**
 * @brief FASTQ Write Base class
 *
 * fastq_parser uses open(), close() and write_spot() methods
 */
class fastq_writer
{
public:
    fastq_writer() {};
    virtual ~fastq_writer() = default;

    virtual void open() {};
    virtual void close() {};

    /**
     * @brief Write the list of reads as one spot
     *
     * @param[in] reads
     */
    virtual void write_spot(const vector<CFastqRead>& reads);

    /**
     * @brief Set user-defined attributes
     *
     * @param[in] name
     * @param[in] value
     */
    void set_attr(const string& name, const string& value) {
        m_attr[name] = value;
    }

    /**
     * @brief Get the attr object
     * value is not changed if attr is not found
     * 
     * @param name 
     * @param value 
     */
    void get_attr(const string& name, string& value) {
        auto it = m_attr.find(name);
        if (it != m_attr.end()) {
            value = it->second;
        }
    }

protected:
    using TAttributeName = string;
    using TAttributeValue = string;
    map<TAttributeName, TAttributeValue> m_attr;  ///< Attributes dictionary
};


void fastq_writer::write_spot(const vector<CFastqRead>& reads)
{
    if (reads.empty())
        return;
    const auto& first_read = reads.front();
    string spot_name = first_read.Spot();
    spot_name += first_read.Suffix();
    cout << "Spot: " << spot_name << "\nreads " << reads.size() <<":\n";
    for (const auto& read : reads) {
        //auto sz = read.Sequence().size();
        cout << "num:" << read.ReadNum() << "(" << (read.Type() == 0 ? "T" : "B") << ")" << "\n";
        //cout << "spot_group:" << read.SpotGroup() << "\n";
        cout << read.Sequence() << "\n";
        cout << "+\n";
        cout << read.Quality() << endl;
    }
}


/**
 * @brief VDB Writer implementation
 *
 * Constructor redirects logging to general_loader
 * Open() method sets up the VDB table using user defin attributes
 *
 *
 */
class fastq_writer_vdb : public fastq_writer
{
public:
    fastq_writer_vdb(ostream& stream, shared_ptr<Writer2> writer );
    ~fastq_writer_vdb();

    void open() override;
    void close() override;
    virtual void write_spot(const vector<CFastqRead>& reads) override;

    const Writer2 & get_writer() const { return * m_writer; }
protected:
    shared_ptr<Writer2> m_writer;    ///< VDB Writer
    std::shared_ptr<spdlog::logger> m_default_logger; ///< Saved default loger
    Writer2::Table SEQUENCE_TABLE;
    Writer2::Column c_NAME;
    Writer2::Column c_SPOT_GROUP;
    Writer2::Column c_PLATFORM;
    Writer2::Column c_READ;
    Writer2::Column c_QUALITY;
    Writer2::Column c_READ_START;
    Writer2::Column c_READ_LEN;
    Writer2::Column c_READ_TYPE;
    Writer2::Column c_READ_FILTER;
    Writer2::Column c_CHANNEL;
    Writer2::Column c_READ_NUMBER;
    uint8_t m_platform{0};
    bool m_is_writing{false};  ///< Flag to indicate if writing was initiated
    string m_tmp_sequence; ///< temp string for sequences 
    string m_tmp_spot; ///< temp string for spots
    vector<uint8_t> m_qual_scores; ///< temp vector for scores
};

//  -----------------------------------------------------------------------------
fastq_writer_vdb::fastq_writer_vdb(ostream& stream, shared_ptr<Writer2> writer = shared_ptr<Writer2>() )
{
    if ( writer.get() == nullptr )
    {
        m_writer.reset( new Writer2(stream) );
    }
    else
    {
        m_writer = writer;
    }

    m_default_logger = spdlog::default_logger();
    auto logger = general_writer_logger_mt("general_writer", m_writer);
    spdlog::set_default_logger(logger);
}

//  -----------------------------------------------------------------------------
fastq_writer_vdb::~fastq_writer_vdb()
{
    if (m_is_writing) {
        close();
    }


}

//  -----------------------------------------------------------------------------
void fastq_writer_vdb::open()
{
    //static const string cSCHEMA = "sra/generic-fastq.vschema";
    //static const string cGENERIC_DB = "NCBI:SRA:GenericFastq:db";

    string name_column_expression = "RAW_NAME";
    get_attr("name_column", name_column_expression);
    bool has_name_column{false};
    if (name_column_expression == "NAME") {
        has_name_column = true;
        name_column_expression = "(ascii)NAME";
    }

    string quality_expression = "(INSDC:quality:text:phred_33)QUALITY";
    get_attr("quality_expression", quality_expression);

    string platform;
    get_attr("platform", platform);
    if (!platform.empty())
        m_platform = stoi(platform);

    string destination{"sra.out"};
    get_attr("destination", destination);
    m_writer->destination(destination);

    string version{"1.0"};
    get_attr("version", version);
    m_writer->info("sharq", version);

    static const string cGENERIC_SCHEMA = "sra/generic-fastq.vschema";
    static const string c454_SCHEMA = "sra/454.vschema";
    static const string cION_TORRENT_SCHEMA = "sra/ion-torrent.vschema";

    static const string cGENERIC_DB = "NCBI:SRA:GenericFastq:db";
    static const string cILLUMINA_DB = "NCBI:SRA:Illumina:db";
    static const string cNANOPORE_DB = "NCBI:SRA:GenericFastqNanopore:db";
    static const string c454_DB = "NCBI:SRA:_454_:db";
    static const string cION_TORRENT_DB = "NCBI:SRA:IonTorrent:db";
    string db = cGENERIC_DB;
    string schema = cGENERIC_SCHEMA;
    switch (m_platform)
    {
    case SRA_PLATFORM_ILLUMINA:
        // use Illumina DB for illumina with standard NAME column
        if (has_name_column)
            db = cILLUMINA_DB;
        break;
    case SRA_PLATFORM_OXFORD_NANOPORE:
        db = cNANOPORE_DB;
        break;
    case SRA_PLATFORM_454: 
        schema = c454_SCHEMA;
        db = c454_DB;
        break;
    case SRA_PLATFORM_ION_TORRENT:
        schema = cION_TORRENT_SCHEMA;
        db = cION_TORRENT_DB;
        break;
    }
    //m_writer->set_attr("schema", schema);
    //m_writer->set_attr("db", db);


    //string schema = cSCHEMA;        
    //string db = cGENERIC_DB;
    //get_attr("schema", schema);
    //get_attr("db", db);
    m_writer->schema(schema, db);

    vector<Writer2::ColumnDefinition> SequenceCols = {
        { "READ",               sizeof(char) }, // sequence literals
//        { "CSREAD",             sizeof(char) }, // string
//        { "CS_KEY",             sizeof(char) }, // one character
        { "READ_START",         sizeof(int32_t) }, //one per read
        { "READ_LEN",           sizeof(int32_t) }, //one per read
        { "READ_TYPE",          sizeof(char) }, // one per read
        { "READ_FILTER",        sizeof(char) }, // one per read '0'or '1'
        { "QUALITY",            sizeof(char), quality_expression.c_str() }, // quality string
        { "NAME",               sizeof(char), name_column_expression.c_str() }, // spotName
        { "SPOT_GROUP",         sizeof(char) }, // barcode
//        { "CLIP_QUALITY_LEFT",  sizeof(int32_t) }, // one per read, default 0
//        { "CLIP_QUALITY_RIGHT", sizeof(int32_t) }, // one per read, default 0
//        { "LABEL",              sizeof(char) }, // concatenated labe string eg. 'FR'
//        { "LABEL_START",        sizeof(int32_t) }, // one per read
//        { "LABEL_LEN",          sizeof(int32_t) }, // one per read
        { "PLATFORM",           sizeof(char) } // platform code
    };
    if ( m_platform == SRA_PLATFORM_OXFORD_NANOPORE )
    {
        SequenceCols.push_back( Writer2::ColumnDefinition( "CHANNEL", sizeof(uint32_t) ) );
        SequenceCols.push_back( Writer2::ColumnDefinition( "READ_NUMBER", sizeof(uint32_t) ) );
    };

    m_writer->addTable("SEQUENCE", SequenceCols);
    SEQUENCE_TABLE = move(m_writer->table("SEQUENCE"));
    c_NAME = move(SEQUENCE_TABLE.column("NAME"));
    c_SPOT_GROUP = move(SEQUENCE_TABLE.column("SPOT_GROUP"));
    c_PLATFORM = move(SEQUENCE_TABLE.column("PLATFORM"));
    c_READ = move(SEQUENCE_TABLE.column("READ"));
    c_QUALITY = move(SEQUENCE_TABLE.column("QUALITY"));
    c_READ_START = move(SEQUENCE_TABLE.column("READ_START"));
    c_READ_LEN = move(SEQUENCE_TABLE.column("READ_LEN"));
    c_READ_TYPE = move(SEQUENCE_TABLE.column("READ_TYPE"));
    c_READ_FILTER = move(SEQUENCE_TABLE.column("READ_FILTER"));
    if ( m_platform == SRA_PLATFORM_OXFORD_NANOPORE )
    {
        c_CHANNEL = move(SEQUENCE_TABLE.column("CHANNEL"));
        c_READ_NUMBER = move(SEQUENCE_TABLE.column("READ_NUMBER"));
    }

    m_writer->beginWriting();
    m_is_writing = true;

}

//  -----------------------------------------------------------------------------
void fastq_writer_vdb::close()
{
    if (m_is_writing && m_writer) {
        m_writer->endWriting();
        m_is_writing = false;
        m_writer->flush();
        spdlog::set_default_logger(m_default_logger);
        //spdlog::set_default_logger(m_default_logger);
        //m_writer.reset();
    }
}

/*
typedef INSDC_SRA_xread_type INSDC_read_type;
enum
{
    // read_type
    SRA_READ_TYPE_TECHNICAL  = 0,
    SRA_READ_TYPE_BIOLOGICAL = 1,

    // orientation - applied as bits, e.g.: type = READ_TYPE_BIOLOGICAL | READ_TYPE_REVERSE
    SRA_READ_TYPE_FORWARD = 2,
    SRA_READ_TYPE_REVERSE = 4,
};
*/
//  -----------------------------------------------------------------------------
void fastq_writer_vdb::write_spot(const vector<CFastqRead>& reads)
{
    if (reads.empty())
        return;
    //auto table = m_writer->table("SEQUENCE");
    const auto& first_read = reads.front();
    m_tmp_spot = first_read.Spot();
    m_tmp_spot += first_read.Suffix();
    c_NAME.setValue(m_tmp_spot);
    c_SPOT_GROUP.setValue(first_read.SpotGroup());
    c_PLATFORM.setValue(m_platform);
    auto read_num = reads.size();
    size_t start  = 0;
    int32_t read_start[read_num];
    int32_t read_len[read_num];
    char read_type[read_num];
    char read_filter[read_num];
    uint32_t channel[read_num];
    uint32_t read_no[read_num];
    m_tmp_sequence.clear();
    read_num = 0;
    for (const auto& read : reads) {
        m_tmp_sequence += read.Sequence();
        m_qual_scores.clear();
        read.GetQualScores(m_qual_scores);
        read_start[read_num] = start;
        auto sz = read.Sequence().size();
        start += sz;
        read_len[read_num] = sz;
        //SRA_READ_TYPE_TECHNICAL = 0;
        //SRA_READ_TYPE_BIOLOGICAL = 1;
        read_type[read_num] = read.Type();
        read_filter[read_num] = (char)read.ReadFilter();
        if ( m_platform == SRA_PLATFORM_OXFORD_NANOPORE )
        {
            channel[read_num] = stoul( read.Channel() );
            read_no[read_num] = stoul( read.NanoporeReadNo() );
        }
        ++read_num;
    }
    std::transform(m_tmp_sequence.begin(), m_tmp_sequence.end(), m_tmp_sequence.begin(), ::toupper);
    c_READ.setValue(m_tmp_sequence);
    c_QUALITY.setValue(m_qual_scores.size(), sizeof(uint8_t), &m_qual_scores[0]);
    c_READ_START.setValue(read_num, sizeof(int32_t), read_start);
    c_READ_LEN.setValue(read_num, sizeof(int32_t), read_len);
    c_READ_TYPE.setValue(read_num, sizeof(char), read_type);
    c_READ_FILTER.setValue(read_num, sizeof(char), read_filter);
    if ( m_platform == SRA_PLATFORM_OXFORD_NANOPORE )
    {
        c_CHANNEL.setValue(read_num, sizeof(uint32_t), channel);
        c_READ_NUMBER.setValue(read_num, sizeof(uint32_t), read_no);
    }
    SEQUENCE_TABLE.closeRow();
}
using json = nlohmann::json;

class fastq_writer_exp : public fastq_writer_vdb
{
public:    
    fastq_writer_exp(const json& ExperimentSpecs, ostream& stream, shared_ptr<Writer2> writer = shared_ptr<Writer2>())
        : fastq_writer_vdb(stream, writer)
    {
            auto j = ExperimentSpecs["EXPERIMENT"]["DESIGN"]["SPOT_DESCRIPTOR"]["SPOT_DECODE_SPEC"]["READ_SPEC"];
            if (j.is_null())
                throw fastq_error("Experiment does not contains READ_SPEC");

            if (j.is_array()) {
                for (auto it = j.begin();it != j.end(); ++it) {           
                    auto& v = *it;
                    auto base_coord = v["BASE_COORD"].get<string>();
                    read_start.push_back(stoi(base_coord) - 1);
                    auto read_class = v["READ_CLASS"].get<string>();
                    m_read_type.push_back((read_class == "Technical Read") ? 0 : 1);
                }
            } else {
                auto base_coord = j["BASE_COORD"].get<string>();
                read_start.push_back(stoi(base_coord) - 1);
                auto read_class = j["READ_CLASS"].get<string>();
                m_read_type.push_back((read_class == "Technical Read") ? 0 : 1);
            }
        
        auto num_reads = read_start.size();
        read_len.resize(num_reads, 0);
        read_filter.resize(num_reads, 0);
    }  


    void write_spot(const vector<CFastqRead>& reads) override 
    {
        if (reads.empty())
            return;
        //auto table = m_writer->table("SEQUENCE");
        const auto& first_read = reads.front();
        m_tmp_spot = first_read.Spot();
        m_tmp_spot += first_read.Suffix();
        c_NAME.setValue(m_tmp_spot);
        c_SPOT_GROUP.setValue(first_read.SpotGroup());
        c_PLATFORM.setValue(m_platform);
        m_tmp_sequence = first_read.Sequence();
        m_qual_scores.clear();
        first_read.GetQualScores(m_qual_scores);
        auto read_num = read_start.size();
        auto sz = m_tmp_sequence.size();
        for (int i = read_num - 1; i >=0; --i) {
            read_len[i] = sz - read_start[i];
            sz -= read_len[i];
        }
        std::transform(m_tmp_sequence.begin(), m_tmp_sequence.end(), m_tmp_sequence.begin(), ::toupper);
        c_READ.setValue(m_tmp_sequence);
        c_QUALITY.setValue(m_qual_scores.size(), sizeof(uint8_t), &m_qual_scores[0]);
        c_READ_START.setValue(read_num, sizeof(int32_t),read_start.data());
        c_READ_LEN.setValue(read_num, sizeof(int32_t), read_len.data());
        c_READ_TYPE.setValue(read_num, sizeof(char), m_read_type.data());
        c_READ_FILTER.setValue(read_num, sizeof(char), read_filter.data());
        SEQUENCE_TABLE.closeRow();
    }
protected:
    vector<int32_t> read_start;
    vector<int32_t> read_len;
    vector<char> m_read_type;
    vector<char> read_filter;


};


#endif
