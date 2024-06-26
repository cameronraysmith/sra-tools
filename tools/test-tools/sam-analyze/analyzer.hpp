#ifndef ANALYZER_H
#define ANALYZER_H

#include "params.hpp"
#include "result.hpp"
#include "sam_db.hpp"
#include "ref_dict.hpp"
#include "spot.hpp"

class analyzer_t {
    private :
        sam_database_t &db;
        const analyze_params_t& params;
        ref_dict_t& ref_dict;
        analyze_result_t result;
        
        analyzer_t( const analyzer_t& ) = delete;

        void analyze_spots( void ) {
            sam_spot_t spot;
            sam_spot_iter_t spot_iter( db );
            while ( spot_iter . next( spot ) ) {
                result . spot_count += 1;
                result . counter_dict . add( spot .count() );
                spot . enter_into_ref_dict( ref_dict );
                switch ( spot . spot_type() ) {
                    case sam_spot_t::SPOT_ALIGNED      : result . fully_aligned += 1; break;
                    case sam_spot_t::SPOT_UNALIGNED    : result . unaligned += 1; break;
                    case sam_spot_t::SPOT_HALF_ALIGNED : result . half_aligned += 1; break;
                }
                spot . count_flags( result . flag_counts, result . flag_problems );
            }
        }

    public :
        analyzer_t( sam_database_t& a_db, const analyze_params_t& a_params, ref_dict_t& a_ref_dict )
            : db( a_db ), params( a_params ), ref_dict( a_ref_dict ) { }

        bool run( void ) {
            std::cerr << "ANALYZE:" << std::endl;
            analyze_spots();
            result . refs_in_use = ref_dict . size();
            result . success = true;
            result . report();
            return result . success;
        }
};

#endif
