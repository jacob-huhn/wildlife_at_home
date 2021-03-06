<?php

$cwd[__FILE__] = __FILE__;
if (is_link($cwd[__FILE__])) $cwd[__FILE__] = readlink($cwd[__FILE__]);
$cwd[__FILE__] = dirname($cwd[__FILE__]);

require_once($cwd[__FILE__] . "/../../../citizen_science_grid/my_query.php");

//require_once($cwd[__FILE__] . '/../../mustache.php/src/Mustache/Autoloader.php');
//Mustache_Autoloader::register();

function set_time_text($time_s) {
    if ($time_s == -1) return '';
    //if ($time_s == -1) return -1;
    else {
        return str_pad(floor($time_s / 3600), 2, '0', STR_PAD_LEFT) . ":" . str_pad(floor(($time_s % 3600) / 60), 2, '0', STR_PAD_LEFT) . ":" . str_pad(($time_s % 60), 2, '0', STR_PAD_LEFT);
    }
}


function get_observations($row_only, $video_id, $user_id, $observation_id, $species_id, $expert_only) {
    $query = "";
    $result = null;
    if ($row_only) {
        $query = "SELECT * FROM timed_observations WHERE id = $observation_id";
        $result = query_wildlife_video_db($query);
    } else {
        $query = "SELECT * FROM timed_observations WHERE video_id = $video_id AND user_id = $user_id ORDER BY start_time, end_time";
        $result = query_wildlife_video_db($query);
    }

    //error_log("query: $query");

    $observations['has_observations'] = false;

    while ($row = $result->fetch_assoc()) {
        $observations['has_observations'] = true;

        $row['start_time_text'] = set_time_text($row['start_time_s']);
        $row['end_time_text'] = set_time_text($row['end_time_s']);

        $query = "SELECT name FROM user WHERE id = " . $row['user_id'];
        $user_result = query_boinc_db($query);
        $user_row = $user_result->fetch_assoc();

        $row['user_name'] = $user_row['name'];

        $selected_tags = explode("#", $row['tags']);
        $row['tags'] = array();
        foreach ($selected_tags as $tag) {
            $current_tag['tag_text'] = $tag;
            $row['tags'][] = $current_tag;
        }

        $query = "SELECT category, name, possible_tags FROM observation_types WHERE id = " . $row['event_id'];
        $type_result = query_wildlife_video_db($query);
        $type_row = $type_result->fetch_assoc();
        $row['event_type'] = $type_row['category'] . " - " . $type_row['name'];

        $event_info = $row;

        if ($type_row['possible_tags'] != '') {
            $event_info['has_tags'] = true;

            $tag_id = 0;
            $possible_tags = explode(", ", $type_row['possible_tags']);
            $tags = array();
            foreach ($possible_tags as $tag) {
                $current_tag['tag_name'] = $tag;
                $current_tag['tag_id'] = $tag_id;

                $tags[] = $current_tag;
                $tag_id++;
            }

            $event_info['possible_tags'] = $tags;
        }

        if ($expert_only == 1) {
//            error_log("EXPERT_ONLY = 1");
            $query = "SELECT id, category, name, instructions FROM observation_types WHERE ";
        } else {
//            error_log("EXPERT_ONLY: " . $expert_only);
            $query = "SELECT id, category, name, instructions FROM observation_types WHERE expert_only = $expert_only AND ";
        }

        if ($species_id == 1) { //sharptailed grouse
            $query .= "sharptailed_grouse = 1";
        } else if ($species_id == 2) { //least tern
            $query .= "least_tern = 1";
        } else if ($species_id == 3) { //piping plover
            $query .= "piping_plover = 1";
        } else {
            return;
        }   
        $query .= " ORDER BY category, id";

        $dropdown_result = query_wildlife_video_db($query);

        $event_info['event_list'] = array();

        if ($row['event_id'] == 0) {
            $event_info['dropdown_text'] = "Select Event <span class='caret'></span>";
        } else {
            $event_info['dropdown_text'] = $type_row['category'] . " - " . $type_row['name'] . " <span class='caret'></span>";
        }

        $prev_category = ''; 
        while ($dropdown_row = $dropdown_result->fetch_assoc()) {
            if ($dropdown_row['category'] != $prev_category) $dropdown_row['new_category'] = true;

            $dropdown_row['event_id'] = $dropdown_row['id'];
            unset($dropdown_row['id']);

            $event_info['event_list'][] = $dropdown_row;
        }   

        $prev_category = $event_info['event_list'][0]['category'];
        $prev_category_key = 0;
        $event_count = 1;
        for ($i = 1; $i < count($event_info['event_list']); $i++) {
//            error_log("prev category: '$prev_category', current: '". $event_info['event_list'][$i]['category'] . "'");

            if (0 != strcmp($event_info['event_list'][$i]['category'], $prev_category) ) { 
//                error_log("    different, event_count is: $event_count");

                $event_info['event_list'][$prev_category_key]['event_count'] = $event_count;
                $event_info['event_list'][$prev_category_key]['new_category'] = true;

                $prev_category = $event_info['event_list'][$i]['category'];
                $prev_category_key = $i; 
                $event_count = 0;
            }   
            $event_info['event_list'][$i]['new_category'] = false;
            $event_info['event_list'][$i]['new_column'] = false;

//            error_log("  checking: $i == " . floor(count($event_info['event_list']) / 2) );
            if ($i == floor(count($event_info['event_list']) / 3) || $i == floor(count($event_info['event_list']) * (2 / 3))) {
                $event_info['event_list'][$i]['new_column'] = true;
//                error_log("    SETTING NEW COLUMN: i = $i");
            }   

            $event_count++;
        }   
        $event_info['event_list'][$prev_category_key]['event_count'] = $event_count;
        $event_info['event_list'][$prev_category_key]['new_category'] = true;

        $observations['observations'][] = $event_info;
    }

    return $observations;
}

function get_timed_observation_table($video_id, $user_id, &$observation_count, $species_id, $expert_only) {
    global $cwd;
    $observations = get_observations(false, $video_id, $user_id, null, $species_id, $expert_only);
    $observation_count = count($observations['observations']);

    $observation_table_template = file_get_contents($cwd[__FILE__] . "/../templates/observation_table_template.html");
    $mustache_engine = new Mustache_Engine;
    return $mustache_engine->render($observation_table_template, $observations);
}

function get_timed_observation_row($observation_id, $species_id, $expert_only) {
    global $cwd;
    $observations = get_observations(true, null, null, $observation_id, $species_id, $expert_only);
    $observations['row_only'] = true;

    $observation_table_template = file_get_contents($cwd[__FILE__] . "/../templates/observation_table_template.html");
    $mustache_engine = new Mustache_Engine;
    return $mustache_engine->render($observation_table_template, $observations);
}

function get_watch_video_interface($species_id, $video_id, $video_file, $animal_id, $user, $start_time, $difficulty) {
    global $cwd;
    $watch_info['video_id'] = $video_id;
    $watch_info['video_file'] = $video_file;
    $watch_info['start_time'] = $start_time;
    $watch_info['animal_id'] = $animal_id;
    $watch_info['trimmed_filename'] = trim(substr($video_file, strrpos($video_file, '/') + 1));
    $watch_info['bossa_total_credit'] = $user['bossa_total_credit'];
    $watch_info['total_events'] = $user['total_events'];
    $watch_info['valid_events'] = $user['valid_events'];
    $watch_info['invalid_events'] = $user['invalid_events'];
    $watch_info['missed_events'] = $user['missed_events'];

    if ($difficulty == 'easy') $watch_info['difficulty_class'] = 'btn-success';
    else if ($difficulty == 'medium') $watch_info['difficulty_class'] = 'btn-warning';
    else if ($difficulty == 'hard') $watch_info['difficulty_class'] = 'btn-danger';

    $watch_info['difficulty_text'] = ucfirst($difficulty);

    $query = "SELECT u_id FROM registration WHERE u_id=" . $user['id'];
    $result = query_wildlife_video_db($query);
    $rows = $result->num_rows;

    if($rows == 0) {
        $watch_info['new_user_survey'] = 1;
    } else {
        if ($user['bossa_total_credit'] >= 86400) {
            $query = "SELECT u_id FROM goldbadge WHERE u_id=" . $user['id'];
            $result = query_wildlife_video_db($query);

            $rows = $result->num_rows;

            if($rows == 0) {
                $watch_info['gold_user_survey'] = 1;
            }
        }
    }

    $watch_interface_template = file_get_contents($cwd[__FILE__] . "/../templates/watch_template.html");
    $mustache_engine = new Mustache_Engine;
    return $mustache_engine->render($watch_interface_template, $watch_info);
}

function get_expert_video_row($species_id, $video_id, $video_file, $animal_id, $start_time, $needs_revalidation, $user) {
    global $cwd;
    $watch_info['video_id'] = $video_id;
    $watch_info['video_file'] = $video_file;
    $watch_info['animal_id'] = $animal_id;
    $watch_info['trimmed_filename'] = trim(substr($video_file, strrpos($video_file, '/') + 1));
    $watch_info['start_time'] = $start_time;
    $watch_info['needs_revalidation'] = $needs_revalidation;

    if (csg_is_special_user($user, false)) {
        $query = "SELECT * FROM expert_observations WHERE video_id = $video_id";
        $result = query_wildlife_video_db($query);

        while ($row = $result->fetch_assoc()) {
            $name_query = "SELECT name FROM user WHERE id = " . $row['user_id'];
            $name_result = query_boinc_db($name_query);
            $name_row = $name_result->fetch_assoc();

            $row['user_name'] = $name_row['name'];

    //        error_log( json_encode($row) );
            $watch_info['old_observations'][] = $row;
        }

        if (count($watch_info['old_observations']) > 0) {
            $watch_info['display_old_observations'] = true;
        }
    } else {
        $watch_info['display_old_observations'] = false;
        $watch_info['regular_user'] = true;
    }

    $query = "";
    if (csg_is_special_user($user, false)) {
        $query = "SELECT timed_observations.*, observation_types.name, observation_types.category FROM timed_observations LEFT JOIN observation_types ON (timed_observations.event_id = observation_types.id) WHERE video_id = $video_id AND user_id != " . $user['id'] . " ORDER BY user_id, start_time";
    } else {
        $query = "SELECT timed_observations.*, observation_types.name, observation_types.category FROM timed_observations LEFT JOIN observation_types ON (timed_observations.event_id = observation_types.id) WHERE video_id = $video_id ORDER BY user_id, start_time";
    }

    $result = query_wildlife_video_db($query);
    while ($row = $result->fetch_assoc()) {
        $name_query = "SELECT name FROM user WHERE id = " . $row['user_id'];
        $name_result = query_boinc_db($name_query);
        $name_row = $name_result->fetch_assoc();

        $row['event_type'] = $row['category'] . " - " . $row['name'];

        $row['user_name'] = $name_row['name'];
        if ($row['expert'] == 1) $row['user_name'] = "<b>" . $row['user_name'] . " (expert)</b>";

        if ($row['report_status'] == 'RESPONDED') {
            $row['responded'] = 1;
        } else if ($row['report_status'] == 'REPORTED') {
            $row['reported'] = 1;
        } else {
            $row['unreported'] = 1;
        }


//        error_log( json_encode($row) );
        $watch_info['other_observations'][] = $row;
    }

    if (count($watch_info['other_observations']) > 0) {
        $watch_info['display_other_observations'] = true;
    }

    $watch_interface_template = file_get_contents($cwd[__FILE__] . "/../templates/row_template.html");
    $mustache_engine = new Mustache_Engine;
    return $mustache_engine->render($watch_interface_template, $watch_info);
}


?>
