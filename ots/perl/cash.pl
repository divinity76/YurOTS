# Generates list of players with their money
use warnings;

for $file (glob "../data/players/*")
{
	next unless $file =~ /xml$/;

	open(FILE, $file);
	@contents = <FILE>;
	$_ = "@contents";
	close(FILE);

	$money = 0;

	while (/item id="2148" count="(\d+)"/g) { $money += ($1); }
	while (/item id="2152" count="(\d+)"/g) { $money += ($1*100); }
	while (/item id="2160" count="(\d+)"/g) { $money += ($1*10000); }

	$cash{$file} = $money;
}

open(OUT, ">cash.txt");
for $file (sort { $cash{$b} <=> $cash{$a} } keys %cash)
{
	$k = $cash{$file};
	if ($k >= 100000) {
		$k = int($k/1000); }
	elsif ($k >= 1000) {
		$k = int($k/100)/10; }
	else {
		$k = int($k/10)/100; }

	$file =~ /.*\/(.*)\.xml/;
	print OUT $k, "k\t", $1, "\n";
}
close(OUT);