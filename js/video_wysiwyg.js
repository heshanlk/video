(function ($) {
  Drupal.behaviors.video_wysiwyg = {
    attach: function (context, settings) {
      $('#video-browser-page div.video-item a').click(function (){
        var nid = $(this).attr('ref');
        $(this).addClass('selected');
        Drupal.settings.wysiwyg.plugins.drupal.video.golbal.selectedId = nid;
        return false;
      });
    },
    videoBrowser: function (onSelect, data, settings, instanceId){
      // popup dialog
      var $dialog = $('<div></div>')
      .load(settings.golbal.url)
      .dialog({
        autoOpen: false,
        title: 'Video Browser',
        width: 640,
        height: 360
      });
      $dialog.dialog('open');
      // add button
      $dialog.dialog({
        buttons: [

        {
          text: "Ok",
          click: function() {
            $(this).dialog("close");
          }
        }
        ]
      });
      
      // close
      $dialog.dialog({
        close: function(event, ui) {
          var nid = Drupal.settings.wysiwyg.plugins.drupal.video.golbal.selectedId;
          if(nid.length)
            onSelect('[content:video:'+nid+']', data, settings, instanceId);
        }
      });
    }
  };
})(jQuery);