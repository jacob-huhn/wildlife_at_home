<?php

header('Content-type: text/plain');

$cwd[__FILE__] = __FILE__;
if (is_link($cwd[__FILE__])) $cwd[__FILE__] = readlink($cwd[__FILE__]);
$cwd[__FILE__] = dirname(dirname($cwd[__FILE__]));

require_once($cwd[__FILE__] . "/../citizen_science_grid/my_query.php");

ini_set("mysql.connect_timeout", 300);
ini_set("default_socket_timeout", 300);

// Get Parameters
parse_str($_SERVER['QUERY_STRING']);

$query = "SELECT event_id, time_to_sec(t.start_time) - time_to_sec(v.start_time) AS start_time, time_to_sec(t.end_time) - time_to_sec(v.start_time) AS end_time FROM timed_observations AS t JOIN video_2 AS v ON v.id = video_id WHERE expert = 1 AND video_id = $video_id AND event_id = 41";
$result = query_wildlife_video_db($query);

while ($row = $result->fetch_assoc()) {
    echo $row['event_id'] . " ";
    echo $row['start_time'] . " ";
    echo $row['end_time'];
    echo "\n";
}

?>
