<?php

//git push origin master (sends changes to server proper)
//git commit -a [filename] ('saves' files)

//chdir("/projects/wildlife/html/user"); //Only for testing

require_once('/../../../home/tdesell/wildlife_at_home/webpage/navbar.php');
require_once('/../../../home/tdesell/wildlife_at_home/webpage/footer.php');
require_once('/../../../home/tdesell/wildlife_at_home/webpage/wildlife_db.php');
require_once('/../../../home/tdesell/wildlife_at_home/webpage/my_query.php');
require_once('/../../../home/tdesell/wildlife_at_home/webpage/user.php');
require_once('/../../../home/tdesell/wildlife_at_home/webpage/boinc_db.php');
//require_once("../inc/bossa.inc");
//require_once("../inc/bossa_impl.inc");


//require_once('/projects/wildlife/html/inc/cache.inc');

require ('/../../../home/tdesell/wildlife_at_home/mustache.php/src/Mustache/Autoloader.php');
Mustache_Autoloader::register();

try
{
	$user = get_user();
	
}
catch(Exception $e)
{
	echo "Error: " . $e->getMessage();
}

$bootstrap_scripts = file_get_contents("/../../../home/tdesell/wildlife_at_home/webpage/bootstrap_scripts.html");

echo "
<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">
<html>
<head>
        <meta charset='utf-8'>
        <title>Wildlife@Home: Expert Observations Statistics</title>

        <link rel='alternate' type='application/rss+xml' title='Wildlife@Home RSS 2.0' href='/rss_main.php'>
        <link rel='icon' href='wildlife_favicon_grouewjn3.png' type='image/x-icon'>
        <link rel='shortcut icon' href='wildlife_favicon_grouewjn3.png' type='image/x-icon'>
		
		<style type=\"text/css\">
			#statbuttons {margin-top: 60px; text-align: center;}
			#nestbuttons {margin-top: 10px; text-align: center;}
			.container {margin-top: 30px;}
			#perdaystats, #durationstats, #timestats {margin: 10px; padding: 5px 20px 20px 20px;}
			.datatable {display: table;}
				#perdaydtcol1 {display: table-column;}
				#perdaydtcol2 {display: table-column;}
					#perdaydata {display: table-cell; width: 50%; vertical-align: top;}
					#perdaygraphcon {display: table-cell; width: 50%; vertical-align: top;}
						#perdaygraph {border: 1px solid #000000; float: right;}
		</style>
		
		<script type=\"text/javascript\" src=\"https://www.google.com/jsapi\"></script>
		<script>
		google.load('visualization', '1.0', {'packages':['corechart']});
		
		function fetchStats(species, nest)
		{
			var data = 'action=goforit&species=' + species;
			
			if(nest != 0)
			{
				data = data + '&nestsite=' + nest;
			}
			
			$.ajax({
				type: 'POST',
				url: 'statback.php',
				data: data,
				success: function(data){
					$('.container').html(data);
					if(species != 1)
					{
						$('#nestbuttons').hide();
					}
					else
					{
						$('#nestbuttons').show();
					}
				}
			});
		}
		
		function numtimeschart(keyval, species) //This makes the graph for the data set: number of recess events/day. Called by statback.php.
		{
			var data = new google.visualization.DataTable(); 
			data.addColumn('string', 'Times a Day');
			data.addColumn('number', 'All Sites');
			data.addRows(keyval); //Note: basestring generated by appropriate function in statback.php.
			
			var title = 'Number of Recess Events Per Day';
			var width = '600';
			
			if(species == 1)
			{
				title = title + ' (Sharp-Tailed Grouse)';
			}
			else if(species == 2)
			{
				title = title + ' (Least Tern)';
				width = '1000';
			}
			else if(species == 3)
			{
				title = title + ' (Piping Plover)';
			}
			
			var options = {'title':title, 'vAxis': {title: 'Number of Days Observed'}, 'hAxis': {title: 'Number in Incubation Recesses Per Day'}, 'bar': {groupwidth: '10%'}, 'width':width, 'height':300};
			//Instantiating and drawing chart
			var chart = new google.visualization.ColumnChart(document.getElementById('perdaygraph'));
			chart.draw(data, options);
		}
		
		</script>

        $bootstrap_scripts

  
";
echo "</head><body>";
$active_items = array(
                    'home' => '',
                    'watch_video' => 'active',
                    'message_boards' => '',
                    'preferences' => '',
                    'about_wildlife' => '',
                    'community' => ''
                );

print_navbar($active_items);

if(is_special_user__fixme($user['id']) || intval($user['id']) == 197 || intval($user['id']) == 1)
{
	//require_once("statback.php"); For final version
	require_once("/../../../home/rvanderclute/wildlife_at_home/webpage/statback.php");
	
	echo "<div class=\"row-fluid\" id=\"statbuttons\">View statistics for: <div class=\"btn-group\" style=\"display: inline;\"><button class=\"btn\" name=\"one\" id=\"one\" onclick=\"fetchStats(1, 0); return false;\">Sharp-Tailed Grouse</button><button class=\"btn\" name=\"two\" id=\"two\" onclick=\"fetchStats(2, 0); return false;\">Least Tern</button><button class=\"btn\" name=\"three\" id=\"three\" onclick=\"fetchStats(3, 0); return false;\">Piping Plover</button></div></div>";
	
	echo "<div class=\"row-fluid\" id=\"nestbuttons\" style=\"display: block;\">Nesting Site: <div class=\"btn-group\" style=\"display: inline;\"><button class=\"btn\" name=\"loczero\" name=\"loczero\" onclick=\"fetchStats(1, 0)\">All</button><button class=\"btn\" name=\"locone\" id=\"locone\" onclick=\"fetchStats(1, 1); return false;\">Belden</button><button class=\"btn\" name=\"loctwo\" id=\"loctwo\" onclick=\"fetchStats(1, 2); return false;\">Blaisdell</button><button class=\"btn\" name=\"locthree\" onclick=\"fetchStats(1, 3); return false;\">Lostwood</button></div></div>";
	
	echo "<br /><br />";
	echo "<div class=\"container\">";
	
	//Default species is Sharp-Tailed Grouse
	$species = 1;

	runRoutine($species, 0);

	echo "</div>";
}
else
{
	"You do not have the permissions to view this page!";
}


echo "</body></html>";
?>