<?php

include 'db_conf.php';

function getDBConn() {
  global $servername;
  global $username;
  global $password;
  global $dbname;
  // Create connection
  $conn = new mysqli($servername, $username, $password, $dbname);
  // Check connection
  if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
  } 
  return $conn;
}

function executeSql($sql, $conn) {
  if ($sql->execute() != TRUE) {
    echo "Error: " . $conn->error;
    return false;
  }
  $sql->close();
  return true;
}


?>
