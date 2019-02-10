var address = window.location.hostname;
// var address = '192.168.1.50';
var urlBase = 'http://' + address;

var websock;
function initWebSocket() {
    websock = new WebSocket('ws://' + address + ':81/');
    websock.onopen = function(evt) { console.log('websock open'); };
    websock.onclose = function(evt) { console.log('websock close'); };
    websock.onerror = function(evt) { console.log(evt); };
    websock.onmessage = function(evt) {
      console.log(evt);
      
      if (evt.data.startsWith('{')) {
        var settings = JSON.parse(evt.data);
        var effectsData = settings.effects;
        // $("#status").html(JSON.stringify(effectsData));
        $("#status").html("Received status.");
        $("#settingsEffect").html(effectsData.map(c => '<option value="'+c.id+'">'+c.name+'</option>').join(''));
        setOptionPower(settings.properties.power)
        $('#settingsBrightness').val(settings.properties.brightness);
        $('#settingsAutoplayDuration').val(settings.properties.autoplay_duration);
        $('#settingsAnimationMode').val(settings.properties.animation_mode).add('selected');
        $('#settingsEffect').val(settings.properties.effect).add('selected');

      } else if (evt.data.includes('settings')) {

        if(evt.data.includes('power')) {
          var selectValue = evt.data.substring(evt.data.indexOf('=') + 1);
          $("#status").html("Option updated : " + evt.data + " => " + selectValue);
          setOptionPower(selectValue === 'on')

        } else if(evt.data.includes('effect')) {
          var selectValue = evt.data.substring(evt.data.indexOf('=') + 1);
          $("#status").html("Option updated : " + evt.data + " => " + selectValue);
          $('#settingsEffect').val(selectValue).attr().add('selected');

        } else if(evt.data.includes('animation_mode')) {
          var selectValue = evt.data.substring(evt.data.indexOf('=') + 1);
          $("#status").html("Option updated : " + evt.data + " => " + selectValue);
          $('#settingsAnimationMode').val(selectValue).attr().add('selected');

        } else if(evt.data.includes('brightness')) {
          var selectValue = evt.data.substring(evt.data.indexOf('=') + 1);
          $("#status").html("Option updated : " + evt.data + " => " + selectValue);
          $('#settingsBrightness').val(selectValue);//.trigger('change');
                    
        } else if(evt.data.includes('autoplay_duration')) {
          var selectValue = evt.data.substring(evt.data.indexOf('=') + 1);
          $("#status").html("Option updated : " + evt.data + " => " + selectValue);
          $('#settingsAutoplayDuration').val(selectValue);//.trigger('change');
                    
        } else {
          $("#status").html("Option updated : " + evt.data);
        }
      } else {
        $("#status").html("Received unknown message.");
      }
    };
}

function setOptionPower(isOn) {
  if(isOn) {
    $("#settingsPowerOn").attr("class", "btn btn-primary");
    $("#settingsPowerOff").attr("class", "btn btn-default");
  } else {
    $("#settingsPowerOff").attr("class", "btn btn-primary");
    $("#settingsPowerOn").attr("class", "btn btn-default");
  }
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

$(document).ready(function() {
  initWebSocket()  
//  addColorButtons();

  $("#settingsPowerOn").click(function() {
    websock.send('settings/power=on');
    setOptionPower(true);
  });

  $("#settingsPowerOff").click(function() {
    websock.send('settings/power=off');
    setOptionPower(false);
  });
  
  $("#settingsBrightness").change(function() {
    websock.send('settings/brightness=' + this.value);
  });

  $("#settingsAutoplayDuration").change(function() {
    websock.send('settings/autoplay_duration=' + this.value);
  });

  $("#settingsAnimationMode").change(function() {
    websock.send('settings/animation_mode=' + this.value);
  });

  $("#settingsEffect").change(function() {
    websock.send('settings/effect=' + this.value);
  });

  $("#status").html("Ready");
});

function addEffectButtons(effect) {
  // await sleep(200);
  // $("#status").html(effect.id + " = " + effect.name);
  var template = $("#effectButtonTemplate").clone();
  template.attr("id", "effect-button-" + effect.id);
  template.text(effect.name);
  template.click(function() {
    //postValue("effectName", effect.name);
    websock.send('settings/effect=' + effect.id);
    //$(".grid-item-color").css("border", "none");
    //$(".grid-item-effect").attr("class", "grid-item-effect btn btn-default");
    //$(this).attr("class", "grid-item-effect btn btn-primary");
  });
}

/*
function addColorButtons() {
  var hues = 25;
  var hueStep = 360 / hues;

  var levels = 10;
  var levelStep = 60 / levels;

  for(var l = 20; l < 80; l += levelStep) {
    for(var h = 0; h < hues; h++) {
      addColorButton(h * hueStep, 100, l);
    }
  }

  $('.grid-color').isotope({
    itemSelector: '.grid-item-color',
    layoutMode: 'fitRows'
  });

}

var colorButtonIndex = 0;

function addColorButton(h, s, l) {
  var color = "hsla(" + h + ", " + s + "%, " + l + "%, 1)"
  var template = $("#colorButtonTemplate").clone();
  template.attr("id", "color-button-" + colorButtonIndex++);
  template.css("background-color", color);
  template.click(function() {
    var rgb = $(this).css('backgroundColor');
    var components = rgbToComponents(rgb);

    $(".grid-item-color").css("border", "none");
    $(this).css("border", "1px solid");

    postColor("solidColor", components);
  });

  $("#colorButtonsRow").append(template);
}

// REST API
function postValue(name, value) {
  $("#status").html("Setting " + name + ": " + value + ", please wait...");
  var body = {};
  body[name] = value;
  $.post(urlBase + "/settings", body, function(data) {
    if (data.name != null) {
      $("#status").html("Set " + name + ": " + data.name);
    } else {
      $("#status").html("Set " + name + ": " + data);
    }
  });
}

function delayPostValue(name, value) {
  clearTimeout(postValueTimer);
  postValueTimer = setTimeout(function() {
    postValue(name, value);
  }, 300);
}

function postColor(name, value) {
  $("#status").html("Setting " + name + ": " + value.r + "," + value.g + "," + value.b + ", please wait...");

  var body = { name: name, r: value.r, g: value.g, b: value.b };

  $.post(urlBase + name + "?r=" + value.r + "&g=" + value.g + "&b=" + value.b, body, function(data) {
    $("#status").html("Set " + name + ": " + data);
  })
  .fail(function(textStatus, errorThrown) { $("#status").html("Fail: " + textStatus + " " + errorThrown); });
}

function delayPostColor(name, value) {
  clearTimeout(postColorTimer);
  postColorTimer = setTimeout(function() {
    postColor(name, value);
  }, 300);
}

function componentToHex(c) {
  var hex = c.toString(16);
  return hex.length == 1 ? "0" + hex : hex;
}

function rgbToHex(r, g, b) {
  return "#" + componentToHex(r) + componentToHex(g) + componentToHex(b);
}

function rgbToComponents(rgb) {
  var components = {};

  rgb = rgb.match(/^rgb\((\d+),\s*(\d+),\s*(\d+)\)$/);
  components.r = parseInt(rgb[1]);
  components.g = parseInt(rgb[2]);
  components.b = parseInt(rgb[3]);

  return components;
}
*/