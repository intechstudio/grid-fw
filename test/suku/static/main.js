$(document).ready(function(){

    console.log("Ready!");
    // jQuery methods go here...

    class Graph {
      constructor(param_id) {
        this.id = param_id;
        this.div = document.getElementById(param_id);
        this.obj = null;
        this.data      = [],
        this.l         = 0,
        this.numvalues = 100,
        this.updates   = 0;        

        for (var i=0; i<this.numvalues; ++i) {
          this.data.push(null);
        }


        this.obj = new RGraph.SVG.Line({
          id: this.id,
          data: this.data,
          options: {
              colors: ['black'],
              linewidth: 2,
              shadow: false,
              marginTop: 0,
              marginLeft: 0,
              marginBottom: 0,
              marginRight: 0,
              backgroundGrid: false,
              xaxisTickmarks: 0,
              yaxisTickmarks: 0,
              yaxis: false,
              xaxis: false,
              yaxisScaleMin: 0,
              yaxisScaleMax: 100,
              yaxisScale: false,
              filled: true,
              filledOpacity: 0.2,
          }
      }).responsive([
         {maxWidth: null,width:100,height:100,options:{textSize:16},css:{'float':'right'},parentCss:{'text-align':'none',width:'100px'}}
      ]);

      }
    }


  
    graphList = [];

    for(var i=0; i<8; i++){

      var newGraph = new Graph("UI_TASK_"+i+"_graph");
      graphList.push(newGraph);

    }


    
    // This function is called repeatedly to draw the Line chart.
    // It only creates the Line chart object once and then stores
    // it in a global variable. On subsequent calls (this function
    // gets called 60 times per second) this global variable is
    // used instead of creating the entire Line chart again.
    function drawGraph (graph_descriptor, param_value)
    {
        // Create the Line chart if the global variable 'obj'
        // doesn't exist.

        RGraph.SVG.clear(graph_descriptor.obj.svg);
      

       
        graph_descriptor.data.push(param_value);
        
        // If the number of points in the data is greater than the numvalues
        // variable take a point off of the front of the array.
        if (graph_descriptor.data.length > graph_descriptor.numvalues) {
          graph_descriptor.data.shift();
        }

        // Give the data to the Line chart and redraw the chart.
        graph_descriptor.obj.originalData[0] = graph_descriptor.data;
        graph_descriptor.obj.draw();
        
    
    }
    


    setInterval(function(){ refresh();} , 100);



    $("input#open").click(function(){

      var port = $("input#port").val();
      var baud = $("input#baud").val();

      open(port, baud);
    
    
    });

    
    function init_minimap(){

      $(".MINIMAP_MODULE").append('<div class="MINIMAP_TEXT"></div>');
      $(".MINIMAP_MODULE").append('<div class="MINIMAP_DEBUG"></div>');
      

    }


    init_minimap();



    function open(param_port, param_baud){

      $.ajax({type: "POST", url: "/serial", data: {port: param_port, baud: param_baud}, success: function(result){

        //alert(result);
      }});

    }

    open("COM18", "2000000");

    function refresh(){
      $.ajax({type: "POST", url: "/get_data", success: function(result){

        $(".MINIMAP_MODULE").each(function(index, value){

          if ($(this).attr("data-timeout") > 3){

            $(this).attr("data-timeout", $(this).attr("data-timeout")-1)
          }
          else if($(this).attr("data-timeout")>0){

            $(this).attr("data-timeout", $(this).attr("data-timeout")-1)
            $(this).children(".MINIMAP_TEXT").html("TIMEOUT");
            

          }
          else{
            $(this).children(".MINIMAP_TEXT").html("");
          }
          

        });


        if (result.length){$("div#console").html("");}

        for(var i=0; i<result.length; i++){

          var obj = JSON.parse(result[i]);

          if (obj.type == "TASK"){
            drawGraph(graphList[0], Math.floor(obj.data[0]/16384*10*100));
            drawGraph(graphList[1], Math.floor(obj.data[1]/16384*10*100));
            drawGraph(graphList[2], Math.floor(obj.data[2]/16384*10*100));
            drawGraph(graphList[3], Math.floor(obj.data[3]/16384*10*100));
            drawGraph(graphList[4], Math.floor(obj.data[4]/16384*10*100));
            drawGraph(graphList[5], Math.floor(obj.data[5]/16384*10*100));
            drawGraph(graphList[6], Math.floor(obj.data[6]/16384*10*100));
            drawGraph(graphList[7], Math.floor(obj.data[7]/16384*10*100));


            $("#UI_TASK_0_value").html(Math.floor(obj.data[0]/16384*10*100) + "%");
            $("#UI_TASK_1_value").html(Math.floor(obj.data[1]/16384*10*100) + "%");
            $("#UI_TASK_2_value").html(Math.floor(obj.data[2]/16384*10*100) + "%");
            $("#UI_TASK_3_value").html(Math.floor(obj.data[3]/16384*10*100) + "%");
            $("#UI_TASK_4_value").html(Math.floor(obj.data[4]/16384*10*100) + "%");
            $("#UI_TASK_5_value").html(Math.floor(obj.data[5]/16384*10*100) + "%");
            $("#UI_TASK_6_value").html(Math.floor(obj.data[6]/16384*10*100) + "%");
            $("#UI_TASK_7_value").html(Math.floor(obj.data[7]/16384*10*100) + "%");

          }
          else if (obj.type == "LOOP"){
            
            $("#UI_LOOP_value").html(obj.data[0]);
            $("#UI_SLOW_value").html(obj.data[1]);
            $("#UI_FAST_value").html(obj.data[2]);
            $("#UI_WARP_value").html(obj.data[3]);

          }    
          else if (obj.type == "HEARTBEAT"){
            //var pos_x = (parseInt(obj.data[0])+4)%4;
            //var pos_y = (parseInt(obj.data[1])+4)%4;


            var pos_x = obj.data[0];
            var pos_y = obj.data[1];
            var hwcfg = obj.data[2];

            

            //console.log("#MINIMAP_X"+pos_x+"_Y"+pos_y+"_HW"+hwcfg);

            $("#MINIMAP_X"+pos_x+"_Y"+pos_y).children(".MINIMAP_TEXT").html("HWCFG: "+hwcfg);
            $("#MINIMAP_X"+pos_x+"_Y"+pos_y).attr("data-timeout", 20);
            
          }     
          else if (obj.type == "MIDI"){
            //var pos_x = (parseInt(obj.data[0])+4)%4;
            //var pos_y = (parseInt(obj.data[1])+4)%4;


            var pos_x = obj.data[0];
            var pos_y = obj.data[1];
            var hwcfg = obj.data[2];

            

            //console.log("#MINIMAP_X"+pos_x+"_Y"+pos_y+"_HW"+hwcfg);

            $("#MINIMAP_X"+pos_x+"_Y"+pos_y).children(".MINIMAP_DEBUG").html("MIDI").stop(1,0).fadeIn(0);
            $("#MINIMAP_X"+pos_x+"_Y"+pos_y).children(".MINIMAP_DEBUG").html("MIDI").fadeOut(500);
            
          }  
          else if (obj.type == "PORT"){


           // console.log(obj.data);

            
          }
          else if (obj.type == "LOG" || obj.type == "WARNING" || obj.type == "ERROR" || obj.type == "TRAP"){

            console.log(JSON.stringify(obj.type) + JSON.stringify(obj.data));

            if (obj.type == "LOG"){
              $("#UI_CONSOLE").append('<div style="color: green">'+obj.data+'</div>');
            }
            else if (obj.type == "WARNING"){
              $("#UI_CONSOLE").append('<div style="color: orange">'+obj.data+'</div>');
            }
            else if (obj.type == "ERROR"){
              $("#UI_CONSOLE").append('<div style="color: red">'+obj.data+'</div>');
            }
            else if (obj.type == "TRAP"){
              $("#UI_CONSOLE").append('<div style="color: black;">'+obj.data+'</div>');
            }

            $('#UI_CONSOLE').animate({scrollTop: $('#UI_CONSOLE').prop("scrollHeight")}, 500);

            
          }
          else if (obj.type == "FRAMEERROR"){
            
            $("#UI_FRAMEERROR_value").html(JSON.stringify(obj));
          }
          else{


            $("div#console").append("<div>"+result[i]+"</div>");
          }

        }
     
        //$("div#console").html(JSON.stringify(result.length));
      }});
    }

    $("#button").click(function(){

      refresh();

    
    
    });




}); 


