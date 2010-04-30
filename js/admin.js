// $Id$

/**
 * @file
 * Adds some show/hide to the admin form to make the UXP easier.
 *
 */

$(document).ready(function() {
	video_hide_all_options();
	$("input[@name='vid_convertor']").change(function() {
		video_hide_all_options();
	});
	
	$('.video_select').each(function() {
		var ext = $(this).attr('rel');
		$('select', this).change(function() {
			if($(this).val() == 'video_play_flv') {
				$('#flv_player_'+ext).show();
			} else {
				$('#flv_player_'+ext).hide();
			}
		});
		if($('select', this).val() == 'video_play_flv') {
			$('#flv_player_'+ext).show();
		}
	});	
});

function video_hide_all_options() {
	$("input[@name='vid_convertor']").each(function() {
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
			$('.video_large_thumbnail img').attr('src', Drupal.settings.basePath + path);
		}
	});

}
