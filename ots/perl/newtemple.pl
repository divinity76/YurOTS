# Changes all players spawn and temple position (useful when changing map)
use warnings;

$x = 160;
$y = 54;
$z = 7;

for $file (glob "../data/players/*")
{
	next unless $file =~ /\.xml$/;

	open(FILE, $file);
	@content = <FILE>;
	$_ = "@content";
	close(FILE);

	s/<spawn x="\d+" y="\d+" z="\d+"\/>/<spawn x="$x" y="$y" z="$z"\/>/;
	s/<temple x="\d+" y="\d+" z="\d+"\/>/<temple x="$x" y="$y" z="$z"\/>/;

	open(FILE, ">$file");
	print FILE;
	close(FILE);
}