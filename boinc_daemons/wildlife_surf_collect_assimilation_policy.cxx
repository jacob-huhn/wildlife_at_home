/*
 * Copyright 2012, 2009 Travis Desell and the University of North Dakota.
 *
 * This file is part of the Toolkit for Asynchronous Optimization (TAO).
 *
 * TAO is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TAO is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TAO.  If not, see <http://www.gnu.org/licenses/>.
 * */

#include <vector>
#include <cstdlib>
#include <string>
#include <fstream>

#include <math.h>

#include "config.h"
#include "util.h"
#include "sched_util.h"
#include "sched_msgs.h"
#include "md5_file.h"
#include "error_numbers.h"
#include "validate_util.h"

#include "stdint.h"
#include "mysql.h"
#include "boinc_db.h"

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>

#include "undvc_common/file_io.hxx"
#include "undvc_common/parse_xml.hxx"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "EventType.hpp"

using namespace std;
using namespace cv;

#define mysql_query_check(conn, query) __mysql_check (conn, query, __FILE__, __LINE__)

void __mysql_check(MYSQL *conn, string query, const char *file, const int line) {
    mysql_query(conn, query.c_str());

    if (mysql_errno(conn) != 0) {
        ostringstream ex_msg;
        ex_msg << "ERROR in MySQL query: '" << query.c_str() << "'. Error: " << mysql_errno(conn) << " -- '" << mysql_error(conn) << "'. Thrown on " << file << ":" << line;
        cerr << ex_msg.str() << endl;
        exit(1);
    }
}

MYSQL *wildlife_db_conn = NULL;

void initialize_database() {
    wildlife_db_conn = mysql_init(NULL);

    //shoud get database info from a file
    string db_host, db_name, db_password, db_user;
    ifstream db_info_file("../wildlife_db_info");

    db_info_file >> db_host >> db_name >> db_user >> db_password;
    db_info_file.close();

    cout << "parsed db info:" << endl;
    cout << "\thost: " << db_host << endl;
    cout << "\tname: " << db_name << endl;
    cout << "\tuser: " << db_user << endl;
    cout << "\tpass: " << db_password << endl;

    if (mysql_real_connect(wildlife_db_conn, db_host.c_str(), db_user.c_str(), db_password.c_str(), db_name.c_str(), 0, NULL, 0) == NULL) {
        cerr << "Error connecting to database: " << mysql_errno(wildlife_db_conn) << ", '" << mysql_error(wildlife_db_conn) << "'" << endl;
        exit(1);
    }
}

// This program needs to place all the feature files into their respective
// directories by tag, species, location/nest, video, event_type.
// A separate program will then take all of the events by type and combine them
// and subtract out all of the overlapping features. This should leave features
// specific to that event. Then these feature files can be sent out as work
// units and matched up in corresponding videos.

//returns 0 on sucess
int assimilate_handler(WORKUNIT& wu, vector<RESULT>& results, RESULT& canonical_result) {
    if (wildlife_db_conn == NULL) initialize_database();

    //need to read wu.xml_doc
    string xml_doc;

    ostringstream oss;
    oss << "SELECT xml_doc FROM workunit WHERE id = " << wu.id;
    string query = oss.str();

    mysql_query(boinc_db.mysql, query.c_str());

    MYSQL_RES *my_result = mysql_store_result(boinc_db.mysql);
    if (mysql_errno(boinc_db.mysql) == 0) {
        MYSQL_ROW row = mysql_fetch_row(my_result);

        if (row == NULL) {
            log_messages.printf(MSG_CRITICAL, "Could not get row from workunit with query '%s'. Error: %d -- '%s'\n", xml_doc.c_str(), mysql_errno(boinc_db.mysql), mysql_error(boinc_db.mysql));
            return 1;
        }

        xml_doc = row[0];
    } else {
        log_messages.printf(MSG_CRITICAL, "Could execute query '%s'. Error: %d -- '%s'\n", xml_doc.c_str(), mysql_errno(boinc_db.mysql), mysql_error(boinc_db.mysql));
        return 1;
    }
    mysql_free_result(my_result);

    /*
     * Now that the workunit xml has been collected, we can parse it for the appropriate information
     */
    string tag_str;

    try {
        tag_str = parse_xml<string>(xml_doc, "tag");
    } catch (string error_message) {
        log_messages.printf(MSG_CRITICAL, "wildlife_surf_collect_assimilation_policy assimilate_handler([RESULT#%d %s]) failed with error: %s\n", canonical_result.id, canonical_result.name, error_message.c_str());
        log_messages.printf(MSG_CRITICAL, "XML:\n'%s'\n", xml_doc.c_str());
        return 1;
        return 0;
    }

    OUTPUT_FILE_INFO fi;
    vector<EventType*> event_types;
    try {
        string events_str = parse_xml<string>(canonical_result.stderr_out, "event_ids");
        stringstream ss(events_str);
        vector<string> event_names;

        string temp;
        std::getline(ss, temp, '\n');
        while(std::getline(ss, temp, '\n')) {
            boost::algorithm::trim(temp);
            log_messages.printf(MSG_DEBUG, "Event id: '%s'\n", temp.c_str());
            event_names.push_back(temp);
        }

        int retval = get_output_file_path(canonical_result, fi.path);
        if (retval) {
            log_messages.printf(MSG_CRITICAL, "wildlife_surf_collect_assimilation_policy: Failed to get output file path: %d %s\n", canonical_result.id, canonical_result.name);
            return 1;
            return retval;
        }

        FileStorage infile(fi.path.c_str(), FileStorage::READ);
        for (unsigned int i=0; i<event_names.size(); i++) {
            EventType *temp = new EventType(event_names[i]);
            try {
                temp->read(infile);
                log_messages.printf(MSG_DEBUG, "wildlife_surf_collect_assimilation_policy: Read in %d, descriptors.\n", temp->getDescriptors().rows);
            } catch(const exception &ex) {
                log_messages.printf(MSG_CRITICAL, "wildlife_surf_collect_assimilation_policy get_data_from_result([RESULT#%d %s) failed with error: %s\n", canonical_result.id, canonical_result.name, ex.what());
                return 1;
            }
            event_types.push_back(temp);
        }
        infile.release();
    } catch (string error_message) {
        log_messages.printf(MSG_CRITICAL, "wildlife_surf_collect_assimilation_policy get_data_from_result([RESULT#%d %s]) failed with error: %s\n", canonical_result.id, canonical_result.name, error_message.c_str());
        log_messages.printf(MSG_CRITICAL, "XML:\n%s\n", canonical_result.stderr_out);
        canonical_result.outcome = RESULT_OUTCOME_VALIDATE_ERROR;
        canonical_result.validate_state = VALIDATE_STATE_INVALID;

        log_messages.printf(MSG_DEBUG, "Returning XML Error for %s\n", canonical_result.name);
        return 0; //Nothing ot assimilate.
    }

    // Here we should have a list of event types.

    cout << "tag: " << tag_str << endl;
    cout << "result name: " << canonical_result.name << endl;

    //get video id
    //result name is video_<video_id>_<time>_<result number>
    string result_name = results[0].name;
/*    string result_name = canonical_result.name;     SHOULD BE THIS */
    uint32_t first_pos = result_name.find("_", 0) + 1;
    uint32_t second_pos = result_name.find("_", first_pos);
    while(!isdigit(result_name[first_pos])) {
        first_pos = second_pos + 1;
        second_pos = result_name.find("_", first_pos);
    }

    if (first_pos == string::npos || second_pos == string::npos) {
        log_messages.printf(MSG_CRITICAL, "wildlife_surf_collect_assimilation_policy assimilate_handler failed with 'malformed result name error', result name: %s\n", result_name.c_str());
        return 1;
    }

    string video_id = result_name.substr(first_pos, (second_pos - first_pos) );

    cout << "parsed video id: '" << video_id << "'" << endl;

    //get the video segment id, species_id and location_id:
    //  SELECT id, species_id, location_id FROM video_segment_2 WHERE video_id = video_id and number = i

    ostringstream full_video_query;
    full_video_query << "SELECT species_id, location_id FROM video_2 WHERE id = '" << video_id  << "'" << endl;

    mysql_query_check(wildlife_db_conn, full_video_query.str());
    MYSQL_RES *video_result = mysql_store_result(wildlife_db_conn);

    cout << " got video result" << endl;

    MYSQL_ROW full_video_row = mysql_fetch_row(video_result);
    string species_id(full_video_row[0]);
    string location_id(full_video_row[1]);

    mysql_free_result(video_result);

    // Here we need to create/insert files for each event type into a stored
    // directory structure.
    for(vector<EventType*>::iterator it = event_types.begin(); it != event_types.end(); ++it) {
        string pathname = "/projects/wildlife/feature_files/" + tag_str + "/" + species_id + "/" + location_id + "/" + video_id + "/";
        string filename = (*it)->getId() + ".desc";

        boost::filesystem::path path(pathname);
        boost::system::error_code returnedError;
        boost::filesystem::create_directories(path, returnedError);

        if(returnedError) {
            log_messages.printf(MSG_CRITICAL, "wildlife_surf_collect_assimilation_policy failed with 'cannot create directories error', result name: %s\n", result_name.c_str());
            return 1;
        }

        string full_filename = pathname + filename;
        cerr << "Writing to: '" <<  full_filename << "'" << endl;
        FileStorage outfile(full_filename, FileStorage::WRITE);
        cerr << "Write: '" << (*it)->getId().c_str() << "'" << endl;
        try {
            log_messages.printf(MSG_DEBUG, "wildlife_surf_collect_assimilation_policy: Write out %d, descriptors.\n", (*it)->getDescriptors().rows);
            (*it)->writeDescriptors(outfile);
            (*it)->writeKeypoints(outfile);
        } catch(const exception &ex) {
            log_messages.printf(MSG_CRITICAL, "wildlife_surf_collect_assimilation_policy write_reslts([RESULT#%d %s) failed with error: %s\n", canonical_result.id, canonical_result.name, ex.what());
            return 1;
        }
        outfile.release();
    }
    return 0;
}
