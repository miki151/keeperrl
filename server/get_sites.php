<?php

include 'db.php';

$conn = getDBConn();

$sql = "select filename, UNIX_TIMESTAMP(a.timestamp) as time, display_name, version," .
"count(distinct b.id) as won_games, count(distinct c.id) as total_games, save_info from retired_sites a " .
"left join event_retired_conquered b on TRIM(TRAILING '.sit' FROM a.filename) = b.retired_id " .
"left join event_retired_loaded c on TRIM(TRAILING '.sit' FROM a.filename) = c.retired_id group by filename;";

header("Content-Type:text/plain");

$result = $conn->query($sql);

if ($result->num_rows > 0) {
  while($row = $result->fetch_assoc()) {
#    echo $row["display_name"] . ",";
    echo $row["filename"] . ",";
    echo $row["time"] . ",";
    echo $row["won_games"] . ",";
    echo $row["total_games"] . ",";
    echo $row["save_info"] . ",";
    echo $row["version"] . "\n";
  }
}

$conn->close();
