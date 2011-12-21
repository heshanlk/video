/**
 * @file
 * Adds some show/hide to the admin form to make the UXP easier.
 *
 */
$(document).ready(function() {
  video_hide_all_options();
  $("input[name='vid_convertor']").change(function() {
    video_hide_all_options();
  });

  // change metadata options
  video_hide_all__metadata_options();
  $("input[name='vid_metadata']").change(function() {
    video_hide_all__metadata_options();
  });

  // change metadata options
  video_hide_all__filesystem_options();
  $("input[name='vid_filesystem']").change(function() {
    video_hide_all__filesystem_options();
  });

  $('.video_select').each(function() {
    var ext = $(this).attr('rel');
    var select = $('select', this);
    
    select.change(function() {
      $('#flv_player_'+ext).hide();
      $('#html5_player_'+ext).hide();
      if($(this).val() == 'video_play_flv') {
        $('#flv_player_'+ext).show();
      } else if($(this).val() == 'video_play_html5') {
        $('#html5_player_'+ext).show();
      }
    });
    if(select.val() == 'video_play_flv') {
      $('#flv_player_'+ext).show();
    } else {
      $('#flv_player_'+ext).hide();
    }
    if(select.val() == 'video_play_html5') {
      $('#html5_player_'+ext).show();
    } else {
      $('#html5_player_'+ext).hide();
    }
  });
});

function video_hide_all_options() {
  $("input[name='vid_convertor']").each(function() {
    var id = $(this).val();
    $('#'+id).hide();
    if ($(this).is(':checked')) {
      $('#' + id).show();
    }
  });
}

function videoftp_thumbnail_change() {
  // Add handlers for the video thumbnail radio buttons to update the large thumbnail onchange.
  $(".video-thumbnails input").each(function() {
    var path = $(this).val();
    if($(this).is(':checked')) {
      var holder = $(this).attr('rel');
      $('.'+holder+' img').attr('src', Drupal.settings.basePath + path);
    }
  });
}

function video_hide_all__metadata_options() {
  $("input[name='vid_metadata']").each(function() {
    var id = $(this).val();
    $('#'+id).hide();
    if ($(this).is(':checked')) {
      $('#' + id).show();
    }
  });
}

function video_hide_all__filesystem_options() {
  $("input[name='vid_filesystem']").each(function() {
    var id = $(this).val();
    $('#'+id).hide();
    if ($(this).is(':checked')) {
      $('#' + id).show();
    }
  });
}
