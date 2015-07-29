# Deletes players which haven't logged for a week and have level below 30
use warnings;
$days = 7;
$minlevel = 30;
$counter = 0;

for $file (glob "../data/players/*")
{
	next unless $file =~ /xml$/;

	open(FILE, $file);
	@content = <FILE>;
	$_ = "@content";
	close(FILE);

	/account="(\d+)"/;
	$account = $1;
	/lastlogin="(\d+)"/;
	$lastlogin = $1;
	/level="(\d+)"/;
	$level = $1;

	if ((time() - $lastlogin) > $days*24*3600 && $lastlogin > 0 && $level < $minlevel)
	{
		unlink("$file");
		unlink("../data/accounts/$account.xml");
		unlink("../data/vip/$account.xml");
		print "$file\n";
		$counter++;
	}
}

print "$counter acccounts deleted\n";
