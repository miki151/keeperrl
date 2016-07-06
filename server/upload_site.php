<?php

include 'db.php';

$conn = getDBConn();

function addDBEntry($filename, $displayname, $version, $save_info) {
  global $conn;
  if (($sql = $conn->prepare("INSERT INTO retired_sites (filename, display_name, save_info, version) VALUES (?, ?, ?, ?)")) != TRUE) {
    echo "Error: " . $conn->error;
    return false;
  }
  $sql->bind_param("sssi", $filename, $displayname, $save_info, $version);

  if ($sql->execute() != TRUE) {
    echo "Error: " . $conn->error;
    return false;
  }
  $sql->close();
  $conn->close();
  return true;
}

$target_dir = "uploads/";
$filename = basename($_FILES["fileToUpload"]["name"]);
$target_file = $target_dir . $filename;
$fileType = pathinfo($target_file,PATHINFO_EXTENSION);
// Check if file already exists
if (file_exists($target_file)) {
    echo "Sorry, this file already exists.";
} else
// Check file size
if ($_FILES["fileToUpload"]["size"] > 5000000) {
    echo "Sorry, your file is too large.";
} else
if($fileType != "sit") {
    echo "Sorry, only KeeperRL retired site files are allowed.";
} else
if (move_uploaded_file($_FILES["fileToUpload"]["tmp_name"], $target_file)) {
  exec("./parse_game --input \"$target_file\" --serial_info --display_name --version", $values, $parse_error);
  if ($parse_error != 0) {
    echo "Error parsing save file";
    exec("rm \"$target_file\"");
  } else
    if (!addDBEntry($filename, $values[0], $values[1], $values[2]))
      exec("rm \"$target_file\"");
} else {
  echo "Sorry, there was an error uploading your file.";
}
?>
