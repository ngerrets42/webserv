<?php
$fname="Bob";
$lname="McGopher";

print_r($_SERVER);
print_r($_REQUEST);
print_r($_POST);
/*
if(isset($_POST['fname']) && !empty($_POST['fname'])){
	$fname = $_POST['fname'];;
}

if(isset($_POST['lname']) && !empty($_POST['lname'])){
	$lname = $_POST['lname'];;
}

$hrname="Bob";
if($hrname == $fname) {
	$hrname="Bert";
}

$ceoname="Hank";
if($ceoname == $fname) {
	$ceoname="Hackerman";
}

$output = <<<EOD
<!doctype html>
<html>
<head>
<title>
Letter of refusal
</title>
</head>
<body>
<p>
Dear $fname $lname,
</p>
<p>
Thanks for your application letter to GopherCorp. We have forwarded your message to HR. This is their response:
</p>
<pre>
Could you please tell $fname $lname to never again submit their application through an automated web form? While we appreciate their effort, imagine what would happen it all applicants would submit their resume like this? Total chaos, let me tell you! Also, could you get me five apples at the grocery store? Thanks, $hrname $lname
</pre>
<p>
As you can see from their response, your application for the job has been refused.
</p>
<p>
We hope this e-mail contains the information you were looking for.
</p>
<hr />
<p>
$ceoname $lname, proud Director of GopherCorp and Vice President of Stop Nepotism!, Inc.
</p>
<h5>
This message has been automatically generator by a Gopher
</h5>
</body>
</html>
EOD;

echo $output;
*/
?>
