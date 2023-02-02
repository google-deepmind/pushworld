/* 
Copyright 2022 DeepMind Technologies Limited

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// Github settings
github_repo = "deepmind/pushworld"
file_source = "https://raw.githubusercontent.com/" + github_repo + "/main/"
repo_contents = "https://api.github.com/repos/" + github_repo + "/contents/"

colors = {
    SELF: "#00DC00",
    SELF_BORDER: "#006E00",  // half the fill color
    SELF_WALL: "#FAC71E",
    SELF_WALL_BORDER: "#7D640F",
    GOAL: null, // transparent
    GOAL_BORDER: "#B90000",
    GOAL_OBJECT: "#DC0000",
    GOAL_OBJECT_BORDER: "#6E0000",
    MOVEABLE: "#469BFF",
    MOVEABLE_BORDER: "#23487F",
    WALL: "#0A0A0A",
    WALL_BORDER: "#050505",
}


// Disable default scrollbar movement from arrow keys
window.addEventListener(
  "keydown",
  (e) => {
    if (
      ["Space", "ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight"].includes(e.code)
    ) {
      e.preventDefault();
    }
  },
  false
);


function convertFileToPushworld(name, filedump) {
    var lines = filedump.split("\n").map(line => line.trim()).filter(line => line);
    var elements = {'w': []};
    var r = 1;  // add 1 for the border wall
    for (var line of lines) {
        var cells = line.split(" ").filter(line => line);
        for (var c=1; c <= cells.length; c++) {
            var cell = cells[c-1].split("+").filter(e => e);
            for (var k=0; k < cell.length; k++) {
                var e = cell[k].toLowerCase();
                if (e != ".") {
                    if (!(e in elements))
                        elements[e] = [];
                    elements[e].push([r,c]);
                }
            }
        }
        r += 1;
    }
    var grid_dimensions = [r+1, c+1];  // add 1 for the border wall

    // Append the border wall pixels
    for (var rr=0; rr <= r; rr++) {
        elements['w'].push([rr, 0]);
        elements['w'].push([rr, c]);
    }
    for (var cc=1; cc < c; cc++) {
        elements['w'].push([0, cc]);
        elements['w'].push([r, cc]);
    }

    var pushworld = {
        name: name,
        moveables: [],
        goals: [],
        walls: [],
        grid_dimensions: grid_dimensions
    };

    var sorted_elements = Object.keys(elements);
    sorted_elements.sort();  // put the actor "a" in front of movables

    for (var e of sorted_elements) {
        var pixels = elements[e];
        var position = get2DMin(pixels);
        pixels = pixels.map(p => subPoints(p, position));

        var [
            edgeChains,
            contractionDirections,
            boundaryPixels
        ] = extractEdgePolygons(pixels);

        obj = {
            id: e,
            position: position,
            edgeChains: edgeChains,
            contractionDirections: contractionDirections,
            fillPixels: pixels,
            boundaryPixels: boundaryPixels,
        }

        if (e == 'a') {
            pushworld.moveables.push(obj);
            obj.fillColor = colors.SELF;
            obj.borderColor = colors.SELF_BORDER;
        } else if (e[0] == 'm') {
            pushworld.moveables.push(obj);
            var goal_name = e.replace('m', 'g');
            if (goal_name in elements) {
                // This is a target object
                obj.goal_position = get2DMin(elements[goal_name]);
                obj.fillColor = colors.GOAL_OBJECT;
                obj.borderColor = colors.GOAL_OBJECT_BORDER;
            } else {
                obj.fillColor = colors.MOVEABLE;
                obj.borderColor = colors.MOVEABLE_BORDER;
            }
        } else if (e[0] == 'g') {
            pushworld.goals.push(obj);
            obj.fillColor = colors.GOAL;
            obj.borderColor = colors.GOAL_BORDER;
        } else if (e == 'w') {
            pushworld.walls.push(obj);
            obj.fillColor = colors.WALL;
            obj.borderColor = colors.WALL_BORDER;
        } else if (e == 'aw') {
            pushworld.walls.push(obj);
            obj.fillColor = colors.SELF_WALL;
            obj.borderColor = colors.SELF_WALL_BORDER;
        }
    }

    pushworld.initial_state = pushworld.moveables.map(m => m.position);

    return pushworld;
}


function extractEdgePolygons(pixels) {
    // Create a list of oriented edges, with 4 edges per pixel
    var edges = [];

    // Directions
    var dirs = [
        [0,1],
        [1,0],
        [0,-1],
        [-1,0]
    ];
    var clock_rot90 = {0:1, 1:2, 2:3, 3:0};  // direction indices
    var counterclock_rot90 = {3:2, 2:1, 1:0, 0:3};

    // Create the edges of each pixel
    for (var p1 of pixels) {
        for (var delta_idx=0; delta_idx < 4; delta_idx++) {
            var p2 = addPoints(p1, dirs[delta_idx]);
            var edge = [p1, p2, clock_rot90[delta_idx]];

            // Remove duplicate edges
            var duplicate_idx = getEdgeIndex(edge, edges, true);
            if (duplicate_idx == -1) {
                edges.push(edge);
            } else {
                edges.splice(duplicate_idx, 1);
            }
            p1 = p2;
        }
    }

    // Walk through the edges to extract all chains. Use the edge chains to determine
    // the contraction directions of each edge point.
    var edgeChains = [];
    var contractionDirections = [];

    while (edges.length > 0) {
        var chain = [];
        var edge = edges.pop();
        var [p1, p2, cd] = edge;
        chain.push([p1, dirs[cd]]);

        while (true) {
            var idx = -1;
            for (var k=0; k < 3 && idx == -1; k++) {
                idx = getEdgeIndex([p2, addPoints(p2, dirs[cd]), null], edges);
                cd = counterclock_rot90[cd];
            }
            if (idx != -1) {
                [p1, p2, cd] = edge = edges.splice(idx, 1)[0];
                chain.push([p1, dirs[cd]]);
            } else {
                break;  // end of this chain
            }
        }

        var points = [];
        var contractDirs = [];
        edgeChains.push(points);
        contractionDirections.push(contractDirs);

        for (var k=0; k < chain.length; k++) {
            points.push(chain[k][0]);
            var prev_k = mod((k - 1), chain.length);
            contractDirs.push(
                clipVector(addPoints(chain[k][1], chain[prev_k][1]), -1, 1)
            );
        }
    }

    var boundaryPixels = pixels;  // hack for now

    return [edgeChains, contractionDirections, boundaryPixels];
}

function mod(n, m) {
  return ((n % m) + m) % m;
}

function clipVector(vec, min, max) {
    return vec.map(e => Math.min(Math.max(min, e), max));
}

function getEdgeIndex(edge, edges, reverse) {
    if (reverse === undefined || reverse === false) {
        var [tp1, tp2, _] = edge;
    } else {
        var [tp2, tp1, _] = edge;
    }
    for (var i=0; i < edges.length; i++) {
        var [p1, p2, _] = edges[i];
        if (p1[0] == tp1[0] && p1[1] == tp1[1] && p2[0] == tp2[0] && p2[1] == tp2[1])
            return i;
    }
    return -1;
}

function get2DMin(pixels) {
    if (pixels.length == 1) return pixels[0];
    var min_x = pixels.reduce((a, b) => ((a[0] < b[0]) ? a : b))[0];
    var min_y = pixels.reduce((a, b) => ((a[1] < b[1]) ? a : b))[1];
    return [min_x, min_y];
}

function addPoints(p1, p2) {
    return [p1[0] + p2[0], p1[1] + p2[1]];
}
function subPoints(p1, p2) {
    return [p1[0] - p2[0], p1[1] - p2[1]];
}

function is2DPointInArray(p, array) {
    for (var o of array) {
        if (o[0] == p[0] && o[1] == p[1]) return true;
    }
    return false;
}


function drawObjects(render_window, grid_dimensions, objects) {
    for (const [obj, pos] of objects) {
        drawPixels(
            render_window,
            grid_dimensions,
            obj.fillPixels,
            pos,
            obj.fillColor
        );
        for (var k=0; k < obj.edgeChains.length; k++) {
            drawEdges(
                render_window,
                grid_dimensions,
                obj.edgeChains[k],
                obj.contractionDirections[k],
                pos,
                obj.borderColor,
                render_window.border_width,
            );
        }
    }
}


function getCanvasSize(ctx) {
    var size = [
        ctx.canvas.getBoundingClientRect().height,
        ctx.canvas.getBoundingClientRect().width
    ];
    return size;
}


function syncCanvasSize(ctx) {
    var size = getCanvasSize(ctx);
    ctx.canvas.height = size[0];
    ctx.canvas.width = size[1];
}


function drawPixels(render_window, grid_dimensions, pixels, offset, color) {
    if (color === null || color === undefined) return;

    var ctx = render_window.ctx;
    var window_offset = render_window.window_offset;
    var window_size = getCanvasSize(ctx);

    var scale = Math.min(
        window_size[0] / grid_dimensions[0],
        window_size[1] / grid_dimensions[1]
    );
    var center_offset = [
        (window_size[0] - scale * grid_dimensions[0]) / 2,
        (window_size[1] - scale * grid_dimensions[1]) / 2,
    ];
    window_offset = addPoints(window_offset, center_offset);

    ctx.fillStyle = color;
    for (var [x,y] of pixels) {
        var abs_x = Math.round((x + offset[0]) * scale + window_offset[0]);
        var abs_y = Math.round((y + offset[1]) * scale + window_offset[1]);
        var abs_x2 = Math.round((x + 1 + offset[0]) * scale + window_offset[0]);
        var abs_y2 = Math.round((y + 1 + offset[1]) * scale + window_offset[1]);
        ctx.fillRect(abs_y, abs_x, abs_y2-abs_y, abs_x2-abs_x);
    }
}


function drawEdges(
    render_window,
    grid_dimensions,
    points,
    contractionDirections,
    offset,
    strokeColor,
    strokeWidth
) {
    var ctx = render_window.ctx;
    var window_offset = render_window.window_offset;
    var window_size = getCanvasSize(ctx);

    if (points.length < 2) return;

    var scale = Math.min(
        window_size[0] / grid_dimensions[0],
        window_size[1] / grid_dimensions[1]
    )
    var center_offset = [
        (window_size[0] - scale * grid_dimensions[0]) / 2,
        (window_size[1] - scale * grid_dimensions[1]) / 2,
    ];
    window_offset = addPoints(window_offset, center_offset);

    var absolute_points = [];
    for (var k=0; k < points.length; k++) {
        var p = points[k];
        var contract = contractionDirections[k];

        var abs_p = [
            (p[0] + offset[0]) * scale + window_offset[0] + (strokeWidth / 2) * contract[0],
            (p[1] + offset[1]) * scale + window_offset[1] + (strokeWidth / 2) * contract[1]
        ];
        absolute_points.push(abs_p);
    }

    ctx.beginPath();
    ctx.moveTo(absolute_points[0][1], absolute_points[0][0]);
    for (var i = 1; i < absolute_points.length; i++) {
        ctx.lineTo(absolute_points[i][1], absolute_points[i][0]);
    }
    ctx.closePath();

    ctx.strokeStyle = strokeColor;
    ctx.lineWidth = strokeWidth;
    ctx.stroke();
}

function drawGrid(render_window, grid_dimensions) {
    var ctx = render_window.ctx;
    var window_offset = render_window.window_offset;
    var window_size = getCanvasSize(ctx);

    var scale = Math.min(
        window_size[0] / grid_dimensions[0],
        window_size[1] / grid_dimensions[1]
    )
    var center_offset = [
        (window_size[0] - scale * grid_dimensions[0]) / 2,
        (window_size[1] - scale * grid_dimensions[1]) / 2,
    ];
    window_offset = addPoints(window_offset, center_offset);

    var strokeWidth = 2;
    ctx.strokeStyle = "#F0F0F0";
    ctx.lineWidth = strokeWidth;

    for (var i=0; i < grid_dimensions[0]; i++) {
        ctx.beginPath();

        var p = [i, 0]
        ctx.moveTo(
            p[1] * scale + window_offset[1],
            p[0] * scale + window_offset[0]
        );

        p = [i, grid_dimensions[1]]
        ctx.lineTo(
            p[1] * scale + window_offset[1],
            p[0] * scale + window_offset[0]
        );

        ctx.closePath();
        ctx.stroke();
    }

    for (var j=0; j < grid_dimensions[1]; j++) {
        ctx.beginPath();

        var p = [0, j]
        ctx.moveTo(
            p[1] * scale + window_offset[1],
            p[0] * scale + window_offset[0]
        );

        p = [grid_dimensions[0], j]
        ctx.lineTo(
            p[1] * scale + window_offset[1],
            p[0] * scale + window_offset[0]
        );

        ctx.closePath();
        ctx.stroke();
    }
}

function move(pushworld, state, displacement) {
    var next_state;
    var [pushed_object_ids, transitive_stopping] = getPushedObjects(pushworld, state, displacement);

    if (transitive_stopping) {
        next_state = state;
    } else {
        next_state = [];
        for (var i=0; i < state.length; i++) {
            var obj = pushworld.moveables[i];
            var pos = state[i];

            if (pushed_object_ids.includes(obj.id)) {
                next_state.push(addPoints(displacement, pos));
            } else {
                next_state.push(pos);
            }
        }
    }
    return [next_state, transitive_stopping];
}

function isGoalState(pushworld, state) {
    var is_solved = true;

    for (var i=0; i < state.length; i++) {
        var obj = pushworld.moveables[i];
        var pos = state[i];
        if ('goal_position' in obj) {
            if (obj.goal_position[0] != pos[0] ||
                obj.goal_position[1] != pos[1]) {
                is_solved = false;
                break;
            }
        }
    }
    return is_solved;
}

function getObjectIDsToPositions(pushworld, state) {
    var id_to_pos = {};

    for (var w of pushworld.walls)
        id_to_pos[w.id] = w.position;

    for (var g of pushworld.goals)
        id_to_pos[g.id] = g.position;

    for (var i=0; i < state.length; i++)
        id_to_pos[pushworld.moveables[i].id] = state[i];

    return id_to_pos;
}

function getPushedObjects(pushworld, state, displacement) {
    var actor = pushworld.moveables[0];
    var frontier = [actor];
    var transitive_stopping = false;
    var pushed_object_ids = [];

    var tangible_objects = [].concat(pushworld.moveables, pushworld.walls);
    var id_to_pos = getObjectIDsToPositions(pushworld, state);

    while (frontier.length > 0 && !transitive_stopping) {
        var obj = frontier.pop();
        if (pushed_object_ids.includes(obj.id)) continue;

        pushed_object_ids.push(obj.id);

        for (other_obj of tangible_objects) {
            if (pushed_object_ids.includes(other_obj.id)) continue;

            // The Self Wall is only visible to the Self entity.
            if (obj.id != 'a' && other_obj.id == 'aw') continue;

            // Is other_obj pushed by obj?
            var new_pos = addPoints(id_to_pos[obj.id], displacement);
            var rel_pos = subPoints(new_pos, id_to_pos[other_obj.id]);

            for (var obj_px of obj.boundaryPixels) {
                var rel_obj_px = addPoints(obj_px, rel_pos);
                if (is2DPointInArray(rel_obj_px, other_obj.boundaryPixels)) {
                    // Collision detected
                    frontier.push(other_obj);

                    if (other_obj.id == 'w') {
                        transitive_stopping = true;
                    } else if (obj.id == 'a' && other_obj.id == 'aw') {
                        transitive_stopping = true;
                    }
                    break;
                }
            }
        }
    }
    return [pushed_object_ids, transitive_stopping];
}


function zip(a, b) {
    return a.map(function(e, i) {
        return [e, b[i]];
    });
}

function repaint(game_instance, show_grid=true) {
    syncCanvasSize(game_instance.render_window.ctx);
    var window_size = getCanvasSize(game_instance.render_window.ctx);

    game_instance.render_window.ctx.clearRect(
        game_instance.render_window.window_offset[1],
        game_instance.render_window.window_offset[0],
        window_size[1],
        window_size[0],
    );

    if (show_grid) {
        drawGrid(game_instance.render_window, game_instance.pushworld.grid_dimensions);
    }

    var current_state = game_instance.state_history[game_instance.state_history.length-1];
    drawObjects(
        game_instance.render_window,
        game_instance.pushworld.grid_dimensions,
        [].concat(
            game_instance.pushworld.walls.map(w => [w, w.position]),
            zip(game_instance.pushworld.moveables, current_state),
            game_instance.pushworld.goals.map(g => [g, g.position]),
        ),
    );
}

function initGame(pushworld) {
    var canvas = $("#play_canvas");
    active_game = {
        pushworld: pushworld,
        state_history: [pushworld.initial_state],
        action_history: "",
        render_window: {
            window_offset: [0, 0],
            ctx: canvas[0].getContext('2d'),
            border_width: 2
        }
    }
    repaint(active_game);
}


var active_game = null;
var active_preview_panel = null;
var loaded_puzzle_groups = [];

function loadPuzzleGroup(group_name) {
    if (loaded_puzzle_groups.includes(group_name)) {
        return;
    }
    loaded_puzzle_groups.push(group_name);

    $(".pushworld_puzzles .preview_panel").each(function(){
        var preview_panel = $(this);
        if (preview_panel.attr("puzzle_group") != group_name) {
            return;
        }

        $.getJSON(
            repo_contents + "benchmark/puzzles/" + preview_panel.attr("puzzle_group"),
            function(result) {
                var prev_list = preview_panel.children(".preview_list");
                prev_list.children(".pw-puzzle-list-loading").css("display", "none");

                var puzzle_index = 1;
                for (file_info of result) {
                    const name = "(" + puzzle_index.toString() + ") " +
                        file_info["name"].slice(0, -4);
                    const preview_div = addPuzzlePreview(preview_panel);
                    preview_div.children(".puzzle-index").html(puzzle_index.toString());
                    puzzle_index += 1;

                    $.ajax({
                        async: true,
                        type: 'GET',
                        url: file_info['download_url'],
                        success: function(puzzle_string) {
                            displayPuzzle(
                                convertFileToPushworld(name, puzzle_string),
                                preview_div,
                                preview_panel
                            );
                        }
                    });
                }
            }
        )
    });
}

document.addEventListener('DOMContentLoaded', () => {
    $('.pushworld_puzzles .all_puzzles').click(() => {
        active_preview_panel.css("display", "inline");
        active_game = null;
        $('.pushworld_puzzles .puzzle_panel').css("display", "none");
    });

    $('.pushworld_puzzles .reset_button').click(() => {
        active_game.state_history = [active_game.pushworld.initial_state];
        active_game.action_history = "";
        repaint(active_game);
    });

    $('.pushworld_puzzles .undo_button').click(() => {
        var n = active_game.state_history.length;
        if (n > 1) {
            active_game.state_history = active_game.state_history.slice(0, n-1);
            active_game.action_history = active_game.action_history.slice(0, n-2);
            console.log(active_game.action_history);
            repaint(active_game);
        }
    });

    $(window).resize(function() {
        if (active_game != null) {
            repaint(active_game);
        }
    });

    $('#instructions_modal button').click(() => {
        $('#instructions_modal').fadeOut(300);
    });
    $('#instructions_modal').click(() => {
        $('#instructions_modal').fadeOut(300);
    });

    $('.select_puzzle_group .btn').click(function(){
        var puzzle_group = $(this).attr("puzzle_group");

        active_preview_panel = $(".preview_panel").filter(function(){
            return $(this).attr("puzzle_group") == puzzle_group;
        });
        $(".select_puzzle_group").css("display", "none");
        active_preview_panel.css("display", "block");
        loadPuzzleGroup(puzzle_group);
    });

    $('.pushworld_puzzles .return_select_puzzle_group').click(() => {
        active_preview_panel.css("display", "none");
        $(".select_puzzle_group").css("display", "block");
        active_preview_panel = null;
    });

    document.onkeydown = function(e){
        e = e || window.event;
        keydownHandler(e.keyCode)
    };

    $("#touch-keypad .up").click(() => {
        keydownHandler('38');
    });
    $("#touch-keypad .down").click(() => {
        keydownHandler('40');
    });
    $("#touch-keypad .left").click(() => {
        keydownHandler('37');
    });
    $("#touch-keypad .right").click(() => {
        keydownHandler('39');
    });
});

function keydownHandler(keycode) {
    if (active_game === null) return;

    var displacements = {'38' : [-1,0], '40' : [1,0], '37' : [0,-1], '39' : [0,1]};
    var action_labels = {'38' : 'U', '40' : 'D', '37' : 'L', '39' : 'R'};

    if (keycode in displacements) {
        var current_state = active_game.state_history[active_game.state_history.length-1];
        if (isGoalState(active_game.pushworld, current_state))
            return;

        var [next_state, transitive_stopping] = move(
            active_game.pushworld,
            current_state,
            displacements[keycode],
        );

        if (!transitive_stopping) {
            active_game.state_history.push(next_state);
            active_game.action_history += action_labels[keycode];
            console.log(active_game.action_history);
            repaint(active_game);

            if (isGoalState(active_game.pushworld, next_state))
                $('#solved_modal').fadeIn(400).delay(500).fadeOut(400);
        }
    }
}

function addPuzzlePreview(preview_panel)
{
    clone = $("#pw-preview-template").clone();
    clone.removeAttr('id');
    clone.appendTo(preview_panel.children(".preview_list").children(".previews"));
    return clone;
}

function displayPuzzle(pushworld, preview_div, preview_panel)
{
    clone = preview_div;
    clone.children(".pw-puzzle-loading").css("display", "none");
    clone.children("canvas").css("display", "block");

    clone.children(".name").html(pushworld.name);
    clone.data("puzzle", pushworld);

    clone.click(pushworld, (event) => {
        var pushworld = event.data;
        $('.pushworld_puzzles .puzzle_panel .title').html(pushworld.name);
        preview_panel.css("display", "none");
        active_preview_panel = preview_panel;
        $('.pushworld_puzzles .puzzle_panel').css("display", "inline");
        initGame(pushworld);
    })

    var canvas = clone.children("canvas")[0];
    var ctx = canvas.getContext('2d');
    pushworld.render_window = {
        window_offset: [0, 0],
        ctx: ctx,
        border_width: 1
    };
    repaint({
        state_history: [pushworld.initial_state],
        pushworld: pushworld,
        render_window: pushworld.render_window
    }, false);
};
