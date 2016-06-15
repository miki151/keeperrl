<?php

include 'db.php';

$conn = getDBConn();

$sql = "select filename, UNIX_TIMESTAMP(timestamp) as time, display_name,version, count(game_won) as total_games, coalesce(sum(game_won), 0) as won_games from retired_games a left join highscores2 b on TRIM(TRAILING '.ret' FROM a.filename) = b.game_id group by filename";

header("Content-Type:text/plain");

$result = $conn->query($sql);
if ($result->num_rows > 0) {
  while($row = $result->fetch_assoc()) {
    echo $row["display_name"] . ",";
    echo $row["filename"] . ",";
    echo $row["time"] . ",";
    echo $row["total_games"] . ",";
    echo $row["won_games"] . ",";
    echo $row["version"] . "\n";
  }
}

$conn->close();
