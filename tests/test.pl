#!/usr/bin/perl

use Data::Dumper;
use Config::Tiny;
use Time::Local;
use DBI;
use constant debug => 1;


$in = "162103";


$m_date = MakeDate($in);

print "debug:$m_date\n";


sub MakeDate {
    my ($m_date) = @_[0];
    if(($m_date =~ /\D/) or (length($m_date) != 6)) {
            $m_date = "000000";
          LOGGER("Date from PMS is not correct");
    }
    $m_date = "20".substr($m_date, 0, 2)."-".substr($m_date, 2, 2)."-".substr($m_date, 4, 2);
    return $m_date;
}


