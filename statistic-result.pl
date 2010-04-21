#!/usr/bin/perl -w
use strict;
use Data::Dumper;

$\ = "\n";
$| = 1;

sub printline {
	print "====================";
}

# form statistic list from log files

my @LOG_SMS = `ls | grep sms.txt`;
my @LOG_CREG = `ls | grep creg.txt`;
my @LOG_CALL = `ls | grep call.txt`;
my @LOG_RANDOM = `ls | grep random.txt`;

sub gettype {
	my $str = shift;
	$str =~ /log-([^-]+)-[^-]+\.txt/;
	$1;
}

printline();

foreach my $sms (@LOG_SMS) {
	my $type = gettype($sms);
	my @res = `sed -n '/success/p' $sms`;
	my $good = scalar @res;
	@res = `sed -n '/error/p' $sms`;
	my $bad = scalar @res;
	print "modem $type of SMS test result: ";
	print "\tgood: $good, bad: $bad. rate: ", 
		  int($good/($bad+$good)*100), "\%";
}

printline();

foreach my $creg_file (@LOG_CREG) {
	my $type = gettype($creg_file);
	open my $fh, "<$creg_file" or die $!;
	my $res = join '',<$fh>;
	close $fh;
	my @count = $res =~ /return is (\d+)/g;
	my %result;
	foreach my $s (@count) {
		$result{$s}++;
	}
	print "modem $type of AT & AT+CREG test result: ";
	foreach my $key (keys %result) {
		print "\treturn ", $key, ": ", $result{$key};
	}
}

printline();

foreach my $call (@LOG_CALL) {
	my $type = gettype($call);
	open my $fh, "<$call" or die $!;
	my @dat = <$fh>;
	close $fh;
	my $good = scalar grep /ATD success/, @dat;
	my $bad = scalar grep /ATD err/, @dat;
	print "modem $type of ATD test result: ";
	print "\tgood: $good, bad: $bad, rate: ", 
		  int($good/($bad+$good)*100), "\%";
}

printline();

foreach my $rand_log (@LOG_RANDOM) {
	my $type = gettype($rand_log);
	open my $fh, "<$rand_log" or die "cannot open fh: $!";
	my @dat = <$fh>;
	close $fh;
	my $dat = join "", @dat;
	my @res = ($dat =~ / - (\S+)/g);
	my %res;
	foreach my $str (@res) {
		$res{$str}++;
	}
	print "modem $type of RANDOM CMD test result: ";
	foreach my $key (keys %res) {
		print "\t", $key, " : ", $res{$key};
	}
}

printline();
