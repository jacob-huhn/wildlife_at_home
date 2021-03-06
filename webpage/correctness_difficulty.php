<?php

$cwd[__FILE__] = __FILE__;
if (is_link($cwd[__FILE__])) $cwd[__FILE__] = readlink($cwd[__FILE__]);
$cwd[__FILE__] = dirname(dirname($cwd[__FILE__]));

//echo $cwd[__FILE__];
require_once($cwd[__FILE__] . "/../citizen_science_grid/header.php");
require_once($cwd[__FILE__] . "/../citizen_science_grid/navbar.php");
require_once($cwd[__FILE__] . "/../citizen_science_grid/footer.php");
require_once($cwd[__FILE__] . "/../citizen_science_grid/my_query.php");
require_once($cwd[__FILE__] . "/webpage/correctness.php");

print_header("Wildlife@Home: Duration vs Difficulty", "", "wildlife");
print_navbar("Projects: Wildlife@Home", "Wildlife@Home", "..");

//echo "Header:";

ini_set("mysql.connect_timeout", 300);
ini_set("default_socket_timeout", 300);

// Get Parameters
parse_str($_SERVER['QUERY_STRING']);

// Set buffer for correctness time (+ or - the buffer value)
if (!isset($buffer)) {
    $buffer = 5;
}

$easy_watch_query = "SELECT id FROM watched_videos AS watch JOIN timed_observations AS obs ON obs.user_id = watch.user_id AND obs.video_id = watch.video_id WHERE difficulty = 'easy'";
$medium_watch_query = "SELECT id FROM watched_videos AS watch JOIN timed_observations AS obs ON obs.user_id = watch.user_id AND obs.video_id = watch.video_id WHERE difficulty = 'medium'";
$hard_watch_query = "SELECT id FROM watched_videos AS watch JOIN timed_observations AS obs ON obs.user_id = watch.user_id AND obs.video_id = watch.video_id WHERE difficulty = 'hard'";
$easy_watch_result = query_wildlife_video_db($easy_watch_query);
$medium_watch_result = query_wildlife_video_db($medium_watch_query);
$hard_watch_result = query_wildlife_video_db($hard_watch_query);

echo "
<div class='containder'>
    <div class='row'>
        <div class='col-sm-12'>
    <script type = 'text/javascript' src='https://www.google.com/jsapi'></script>
    <script type = 'text/javascript'>
        google.load('visualization', '1', {packages:['corechart']});
        google.setOnLoadCallback(drawChart);

        function getDate(date_string) {
            if (typeof date_string === 'string') {
                var a = date_string.split(/[- :]/);
                return new Date(a[0], a[1]-1, a[2], a[3] || 0, a[4] || 0, a[5] || 0);
            }
            return null;
        }

        function drawChart() {
            var container = document.getElementById('chart_div');
            var data = new google.visualization.arrayToDataTable([
";

function standard_deviation($sample){
    if(is_array($sample)){
        $mean = array_sum($sample) / count($sample);
        foreach($sample as $key => $num) $devs[$key] = pow($num - $mean, 2);
        return sqrt(array_sum($devs) / (count($devs) - 1));
    }
}

$elements = array();
while ($easy_watch_row = $easy_watch_result->fetch_assoc()) {
    array_push($elements, getBufferCorrectness($easy_watch_row['id'], $buffer));
}
sort($elements);
$size = sizeof($elements);
$avg = array_sum($elements) / $size;
$std = standard_deviation($elements);
if ($size > 0) {
    echo "[";
    echo "'Easy - $size'";
    echo ",";
    echo $elements[0];
    echo ",";
    echo $avg - $std;
    echo ",";
    echo $avg + $std;
    echo ",";
    echo $elements[$size - 1];
    echo "],";
}

$elements = array();
while ($medium_watch_row = $medium_watch_result->fetch_assoc()) {
    array_push($elements, getBufferCorrectness($medium_watch_row['id'], $buffer));
}
sort($elements);
$size = sizeof($elements);
$avg = array_sum($elements) / $size;
$std = standard_deviation($elements);
if ($size > 0) {
    echo "[";
    echo "'Medium - $size'";
    echo ",";
    echo $elements[0];
    echo ",";
    echo $avg - $std;
    echo ",";
    echo $avg + $std;
    echo ",";
    echo $elements[$size - 1];
    echo "],";
}

$elements = array();
while ($hard_watch_row = $hard_watch_result->fetch_assoc()) {
    array_push($elements, getBufferCorrectness($hard_watch_row['id'], $buffer));
}
sort($elements);
$size = sizeof($elements);
$avg = array_sum($elements) / $size;
$std = standard_deviation($elements);
if ($size > 0) {
    echo "[";
    echo "'Hard - $size'";
    echo ",";
    echo $elements[0];
    echo ",";
    echo $avg - $std;
    echo ",";
    echo $avg + $std;
    echo ",";
    echo $elements[$size - 1];
    echo "],";
}

echo "
                ], true);

";
echo "
            var options = {
                legend: 'none'

            };

            var chart = new google.visualization.CandlestickChart(document.getElementById('chart_div'));

            chart.draw(data, options);
        }
    </script>

            <h1>Correctness vs Difficulty</h1>

            <div id='chart_div' style='margin: auto; width: auto; height: 500px;'></div>

            <h2>Parameters: (portion of the URL after a '?')</h2>
            <dl>
                <dt>buffer=</dt>
                <dd>The error in either direction allowed for two events to be matched. The default value is 5.</dd>
            </dl>
            

            <h2>Description:</h2>
            <p>This candlestick chart shows the distribution of user correctness vs their perceived difficulty of a video. Correctness in this case is determined by the number of events in their observation that matched an expert event divided by the total number of events they observed for that video.
            <p>In order to collect this data we discard all vidoes that do not have an expert observation or the expert observation is invalid. This is done by getting a list of all event types and then counting the total number of user events that have a matchins event and dividing it by the number of user events of that type that have an valid expert observation for that video.</p>

        </div>
    </div>
</div>
";

print_footer("Travis Desell, 'Travis Desell, Susan Ellis-Felege and the Wildlife@Home Team'", "Travis Desell, Susan Ellis-Felege");

echo "
    </body>
</html>
";
?>
