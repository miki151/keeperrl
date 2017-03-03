<?php

include 'db.php';


$conn = getDBConn();

$boardId = $_GET["boardId"];

$sql = "select text, author from event_message where board_id = ".mysqli_real_escape_string($conn, $boardId) . " order by timestamp limit 100";

header("Content-Type:text/plain");

$result = $conn->query($sql);

if ($result->num_rows > 0) {
  while($row = $result->fetch_assoc()) {
    echo rawurlencode($row["text"]) . ",";
    echo rawurlencode($row["author"]) . "\n";
  }
}

$conn->close();
