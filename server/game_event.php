<?php

include 'db.php';

$conn = getDBConn();

function addRetiredConquered($gameId, $retired_id, $player_name) {
  global $conn;
  $sql = $conn->prepare("INSERT INTO event_retired_conquered (game_id, retired_id, player_name) VALUES (?, ?, ?)");
  $sql->bind_param("sss", $gameId, $retired_id, $player_name);
  executeSql($sql, $conn);
}

function addRetiredLoaded($gameId, $retired_id, $player_name) {
  global $conn;
  $sql = $conn->prepare("INSERT INTO event_retired_loaded (game_id, retired_id, player_name) VALUES (?, ?, ?)");
  $sql->bind_param("sss", $gameId, $retired_id, $player_name);
  executeSql($sql, $conn);
}

function addTurn($params) {
  global $conn;
  $sql = $conn->prepare("INSERT INTO event_turn (game_id, turn) VALUES (?, ?)");
  $sql->bind_param("si", $params["gameId"], $params["turn"]);
  executeSql($sql, $conn);
}

function addCampaignStarted($params) {
  global $conn;
  $sql = $conn->prepare("INSERT INTO event_campaign_started (game_id, main, lesser, allies, retired, install_id, game_type, player_role) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
  $sql->bind_param("siiiisss", $params["gameId"], $params["main"], $params["lesser"], $params["allies"],
      $params["retired"], $params["installId"], $params["game_type"], $params["player_role"]);
  executeSql($sql, $conn);
}

function addMessage($params) {
  global $conn;
  $sql = $conn->prepare("INSERT INTO event_message (game_id, board_id, author, text) VALUES (?, ?, ?, ?)");
  $sql->bind_param("siss", $params["gameId"], $params["boardId"], $params["author"], $params["text"]);
  executeSql($sql, $conn);
}


$eventType = $_POST["eventType"];

if ($eventType == "retiredConquered")
  addRetiredConquered($_POST["gameId"], $_POST["retiredId"], $_POST["playerName"]);
if ($eventType == "retiredLoaded")
  addRetiredLoaded($_POST["gameId"], $_POST["retiredId"], $_POST["playerName"]);
if ($eventType == "turn")
  addTurn($_POST);
if ($eventType == "boardMessage")
  addMessage($_POST);
if ($eventType == "campaignStarted")
  addCampaignStarted($_POST);

$conn->close();
?>
