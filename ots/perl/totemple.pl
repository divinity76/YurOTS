# Teleports all players to their temple
use warnings;

for $file (glob "../data/players/*")
{
	next unless $file =~ /\.xml$/;

	open(FILE, $file);
	@content = <FILE>;
	$_ = "@content";
	close(FILE);

	if (/temple x="(\d+)" y="(\d+)" z="(\d+)"\/>/)
	{
		$x = $1;
		$y = $2;
		$z = $3;
		s/<spawn x="\d+" y="\d+" z="\d+"\/>/<spawn x="$x" y="$y" z="$z"\/>/;
	}

	open(FILE, ">$file");
	print FILE;
	close(FILE);
}