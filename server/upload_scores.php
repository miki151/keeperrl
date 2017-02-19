<?php

include 'db.php';

function addDBEntry($conn, $game_id, $player_name, $world_name, $game_result, $game_won, $points, $turns, $game_type, $player_role, $version) {

  $sql = $conn->prepare("INSERT INTO highscores (game_id, player_name, world_name, game_result, game_won, points, turns, game_type, player_role, version) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
  $sql->bind_param("ssssiiissi", $game_id, $player_name, $world_name, $game_result, $game_won, $points, $turns, $game_type, $player_role, $version);

  if ($sql->execute() != TRUE) {
    echo "Error: " . $sql->error;
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
  exec(escapeshellcmd("./parse_game --input \"$filepath\" --highscores"),
      $values, $parse_error);
  if ($parse_error != 0) {
    echo "Error parsing highscore file";
  } else {
    foreach($values as $data) {
      $row_data = explode(',', $data);
    //echo "inserting $row_data[0], $row_data[1], $row_data[2], $row_data[3], $row_data[4]" . "\n";
      addDBEntry($dbConn, $row_data[0], $row_data[1], $row_data[2], $row_data[3], $row_data[4], $row_data[5], $row_data[6], $row_data[7], $row_data[8], $row_data[9]);
  }
  }
}
$dbConn->close();
?>
