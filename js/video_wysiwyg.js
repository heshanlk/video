(function ($) {
  Drupal.behaviors.video_wysiwyg = {
    attach: function (context, settings) {
      $('.video-file-browser div.view div.view-content div.views-row').click(function (){
        Drupal.settings.wysiwyg.plugins.drupal.video.golbal.selectedId = 1;
      //        console.log();
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
          //          alert(Drupal.settings.video.selectedId);
          console.log(Drupal.settings);
          //          alert('okiasss');
          //          console.log(settings);
          onSelect('[content:video]', data, settings, instanceId);
        //          onSelect('[content:video]', data, settings, instanceId);
        }
      });
    }
  };
})(jQuery);