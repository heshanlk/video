
CREATE TABLE IF NOT EXISTS `video_queue` (
  `queue_id` int(11) unsigned NOT NULL auto_increment,
  `queue_user_profile_id` int(11) unsigned NOT NULL,
  `queue_user_requires_notification` enum('no','yes') NOT NULL default 'no',
  `queue_video_filename` varchar(120) NOT NULL,
  `queue_video_status` enum('pending','processing','processed','failed') NOT NULL default 'pending',
  `queue_video_output_dir` varchar(255) NOT NULL,
  `queue_failure_reason` varchar(255) NOT NULL,
  `queue_addition_date` datetime NOT NULL default '0000-00-00 00:00:00',
  `queue_processing_date` datetime NOT NULL default '0000-00-00 00:00:00',
  `queue_processed_date` datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (`queue_id`),
  KEY `queue_addition_date` (`queue_addition_date`),
  KEY `queue_processed_date` (`queue_processed_date`),
  KEY `queue_processing_date` (`queue_processing_date`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=2 ;
