<?php
error_reporting();
$uploaddir = '/var/www/upload/';
$uploadfile = $uploaddir . basename($_POST['filename']['filename']);

echo '<pre>';
if (move_uploaded_file($_POST['filename']['file'], $uploadfile)) {
    echo "File is valid, and was successfully uploaded.\n";
} else {
    echo "Possible file upload attack!\n";
}

echo 'Here is some more debugging info:';
print_r($_POST);
print_r($_FILES);
print_r($_SERVER);


print "</pre>";

?>