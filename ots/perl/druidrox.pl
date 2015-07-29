# Removes immunity to poison from all monsters
use warnings;

for $file (glob "../monster/*")
{
	next unless $file =~ /\.xml$/;

	open(FILE, $file);
	@content = <FILE>;
	$_ = "@content";
	close(FILE);

	s/<defense immunity="poison"\/>/<!-- <defense immunity="poison"\/> -->/;

	open(FILE, ">$file");
	print FILE;
	close(FILE);
}
