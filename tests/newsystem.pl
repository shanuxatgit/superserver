#!/usr/bin/perl

use Data::Dumper;
use Config::Tiny;
use Time::Local;
use DBI;
use constant debug => 1;

# Create and read a config file
our $Config = Config::Tiny->new;
    $Config = Config::Tiny->read( 'base_server.cfg' );

our $defaultUMDuration = '0000-00-00 00:02:00';


if($logFile){
        open LOGFILE, "+>>", $logFile;
} else {
        *LOGFILE = *STDOUT;
}


&PmsCommandHandler(@ARGV[0]);


sub PmsCommandHandler {
    my $dbh = DBI->connect("dbi:mysql:database=$Config->{_}->{database};host=$Config->{_}->{hostname};", $Config->{_}->{username}, $Config->{_}->{password});
    die "$!" unless $dbh;

    my (@fidelioMessage) = split(/\|/, $_[0]);
    DEBUG("$fidelioMessage[0]\n");
    $command = $fidelioMessage[0];

    foreach $st (@fidelioMessage) {
            my ($field) = substr($st, 0, 2);
            $all{$field} = substr($st, 2, length($st) - 2);
            #print "all-field: $all{$field}\n";
        if($field eq "DA"){
            $m_date = MakeDate($all{'DA'});
        }
        if ($field eq "TI"){
            $m_time = MakeTime($all{'TI'});
        }   
    }
            #DEBUG("DEBUG One: $all{'DA'}\n");
    if(($command eq "WR") or ($command eq "WD")) { # Wakeup request
        $dateTime = $m_date.' '.$m_time;
        my $sqlQuery = "UPDATE Room SET WakeUp = '".$dateTime."' WHERE RoomID = ".$all{'RN'};
            $dbh->do($sqlQuery);
        DEBUG("DEBUG RoomName: [$all{'RN'}] $dateTime $sqlQuery\n");
        LOG("From Perl PmsCommandHandler called with command $command SET WakeUp time for Room: $all{'RN'} to $dateTime\n");

    } elsif ($command eq "WC") {                                                                        # delete alarm
        my $sqlQuery = "UPDATE Room SET WakeUp = '0000-00-00 00:00:00' WHERE RoomID = ".$all{'RN'};
            $dbh->do($sqlQuery);
        LOG("From Perl PmsCommandHandler called with command $command CANCEL WakeUp call for Room: $all{'RN'}\n");

    } elsif ($command eq "XL") {                                                                                # new message
        LOG("From Perl PmsCommandHandler called with command $command Message send to Room: $all{'RN'}\n");
        #message priority
        $priority = 0;
        $msgDuration = '0000-00-00 00:00:00';
            if(substr($all{'MT'}, 0, 1) eq "*") {
                $priority = 6;
            $msgDuration = $defaultUMDuration; 
            LOG("From Perl PmsCommandHandler Message type is URGENT with priority $priority\n");
            $all{'MT'} =~ s/^\*//;
            } elsif (substr($all{'MT'}, 0, 1) eq "!") {
                $priority = 2;
        }

        if(!$all{'MS'}){ $all{'MS'} = 'Message from Front Desk'; }
	    $dbh->do(q{SET NAMES "cp1251"});
            $sth = $dbh->prepare("INSERT INTO `Message` ( `RoomID` , `MsgID` , `Priority` , `Subject` , `Text` , `Status` , `PostDate` , `OpenDate` , `PostEmpID` , `PMSMsgID`, MessageType, IsActive) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
            $sth->execute($all{'RN'}, $all{'MI'}, $priority, $all{'MS'}, $all{'MT'}, "0", $m_date." ".$m_time, $msgDuration, "0", $all{'MI'}, "in", "TRUE");
            $sth->finish();

    } elsif ($command eq "XD") {                                                                        # delete message
        LOG("From Perl PmsCommandHandler called with command $command DELETE Message with ID $all{'MI'} for Room: $all{'RN'}\n");
            my $sth = $dbh->prepare("DELETE FROM `Message`  WHERE `RoomID` = ? AND `PMSMsgID` = ?;");
            $sth->execute($all{'RN'}, $all{'MI'});
            $sth->finish();

    } elsif ($command eq "XM") {

    } elsif ($command eq "XT") {
    }
  $dbh->disconnect();
}

sub MakeDate {
    my ($m_date) = @_[0];
    if(($m_date =~ /\D/) or (length($m_date) != 6)) {
            $m_date = "000000";
        #  LOGGER("Date from PMS is not correct");
    }
    $m_date = "20".substr($m_date, 0, 2)."-".substr($m_date, 2, 2)."-".substr($m_date, 4, 2);
    return $m_date;
}

sub MakeDate {
    my ($m_date) = @_[0];
    if(($m_date =~ /\D/) or (length($m_date) != 6)) {
            $m_date = "000000";
        #  LOGGER("Date from PMS is not correct");
    }
    $m_date = "20".substr($m_date, 0, 2)."-".substr($m_date, 2, 2)."-".substr($m_date, 4, 2);
    return $m_date;
}

sub MakeTime {
    my ($m_time) = @_[0];
    if(($m_time =~ /\D/)  or (length($m_time) != 6)) {
            $m_time = "000000";
        # LOGGER("Time from PMS is not correct");
    }
    $m_time = substr($m_time, 0, 2).":".substr($m_time, 2, 2).":".substr($m_time, 4, 2);
}



sub LOGGER {
    # Gets currect time and appends data to the log
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
    my($db_time) = ($year+1900)."-".sprintf("%02d", ($mon+1))."-".sprintf("%02d", $mday)." ".sprintf("%02d", $hour).":".sprintf("%02d", $min).":".sprintf("%02d", $sec);
    print "$db_time: @_";
}
sub DEBUG {
    # Gets currect time and appends data to the log
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
    my($db_time) = ($year+1900)."-".sprintf("%02d", ($mon+1))."-".sprintf("%02d", $mday)." ".sprintf("%02d", $hour).":".sprintf("%02d", $min).":".sprintf("%02d", $sec);
    print "@_" if debug;
}
sub LOG {
    # Gets currect time and appends data to the log
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
    my($db_time) = ($year+1900)."-".sprintf("%02d", ($mon+1))."-".sprintf("%02d", $mday)." ".sprintf("%02d", $hour).":".sprintf("%02d", $min).":".sprintf("%02d", $sec);
    print LOGFILE "Message from newsystem.pl: $db_time: @_";
}
