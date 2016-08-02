<?php

include 'db.php';

function addDBEntry($conn, $game_id, $player_name, $world_name, $game_result, $game_won, $points, $turns, $game_type) {

  $sql = $conn->prepare("INSERT INTO highscores2 (game_id, player_name, world_name, game_result, game_won, points, turns, game_type) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
  $sql->bind_param("ssssiiii", $game_id, $player_name, $world_name, $game_result, $game_won, $points, $turns, $game_type);

  if ($sql->execute() != TRUE) {
    echo "Error: " . $conn->error;
    return false;
  }
  $sql->close();
  return true;
}

$dbConn = getDBConn();

$filepath = $_FILES["fileToUpload"]["tmp_name"];
// Check file size
header("Content-Type:text/plain");
if ($_FILES["fileToUpload"]["size"] > 100000) {
  echo "Sorry, your file is too large.";
} else {
  $txt_file    = file_get_contents($filepath);
  $rows        = explode("\n", $txt_file);
  foreach($rows as $row => $data) {
    $row_data = explode(',', $data);
//    echo "inserting $row_data[0], $row_data[1], $row_data[2], $row_data[3], $row_data[4]" . "\n";
    addDBEntry($dbConn, $row_data[0], $row_data[1], $row_data[2], $row_data[3], $row_data[4], $row_data[5], $row_data[6], $row_data[7]);
  }
}
$dbConn->close();
?>
