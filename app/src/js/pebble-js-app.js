Pebble.addEventListener("ready", function(e) {
    console.log("JS vivo " + e.ready);
    console.log(e.type);
});

Pebble.addEventListener("showConfiguration", function() {
    Pebble.openURL("http://tomfox91.github.io/LectureCountdown/conf/");
});

Pebble.addEventListener("webviewclosed", function(e) {
    var conf = decodeURIComponent(e.response);
    
    if (conf) {
        console.log("Conf: " + conf);
        conf = JSON.parse(conf);
        
        var cmds = ['mon', 'tue', 'wed', 'thu', 'fri', 'sat', 'sun', 'override'];
        var i = -1;
        var retry = 0;
        
        function retry(callback) {
            retry++;
            if (retry <= 10) {
                i--;
                console.warn("Received NACK #" + retry + ", retrying");
                callback(null);
            } else {
                console.error("Received >10 NACK");
            }
        }
        
        function ackhandler(trans) {
            i++;
            
            if (i < 7) {
                var inp = conf[cmds[i]];
                var outp = [];
                
                for (var j = 0; j < inp.length; j++) {
                    outp.push(Math.floor(inp[j]/256));
                    outp.push(inp[j]%256);
                }
                
                var mess = {}                
                mess[cmds[i]] = outp;
                
                console.log("Sending: " + JSON.stringify(mess));
                
                Pebble.sendAppMessage(mess, ackhandler, function() {
                    retry(ackhandler);
                });

            } else {
                var mess = {override: conf.override};
                
                console.log("Sending: " + JSON.stringify(mess));

                
                Pebble.sendAppMessage(mess, function() {
                    console.log("All configuration sent");
                }, function() {
                    retry(ackhandler);
                });
            }
        }
        
        ackhandler(null);
    }
});
