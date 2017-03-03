<?php

include 'db.php';

$dbConn = getDBConn();

$version = $dbConn->real_escape_string($_GET["version"]);

$sql = "select * from highscores where version = " . $version;

header("Content-Type:text/plain");

$result = $dbConn->query($sql);
if ($result->num_rows > 0) {
  while($row = $result->fetch_assoc()) {
    echo $row["game_id"] . ",";
    echo $row["player_name"] . ",";
    echo $row["world_name"] . ",";
    echo $row["game_result"] . ",";
    echo $row["game_won"] . ",";
    echo $row["points"] . ",";
    echo $row["turns"] . ",";
    echo $row["game_type"] . ",";
    echo $row["player_role"] . ",";
    echo $row["version"] ."\n";
  }
}
$dbConn->close();
