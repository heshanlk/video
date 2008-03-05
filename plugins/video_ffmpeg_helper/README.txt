FFMPEG Video.module helper
==========================

This helper module facilitates uploading new videos using the video module. It
features a batch processing queue for videos to be transcoded and automatic
thumbnail generation.

Install instructions
--------------------

1. Activate the video_ffmpeg_helper module
2. Setup it's advanced options to meet your needs
3. Move (or symlink) video_render.php and video_scheduler.php into your Drupal root
4. Edit the first line of those files to meet your needs
5. Check permissions of the files and folders (/tmp/video and files/* must be writable by the webserver or the user executling the cron job)
6. You now have two options to execute the video_scheduler.php script:

  6.1 (default) Enable the execution of video_scheduler.php using standard drupal cron.
  
  6.2 Schedule the execution of video_scheduler.php using unix cron

    The crontab should look something like this:
    
    # m     h       dom     mon     dow     user            command
    */1     *       *       *       *       www-data        cd /var/www/filmforge/drupal ; php video_scheduler.php
    
    Note that the video_scheduler doesn't produce any output and cannot be called
    from the web. It will, however, put some information in the watchdog.
  
  
