#include "PongMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

PongMode::PongMode() {

	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of PongMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 + 4*1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1,1);
		std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}
}

PongMode::~PongMode() {

	//----- free OpenGL resources -----
	glDeleteBuffers(1, &vertex_buffer);
	vertex_buffer = 0;

	glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
	vertex_buffer_for_color_texture_program = 0;

	glDeleteTextures(1, &white_tex);
	white_tex = 0;
}

bool PongMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	// -- player movements --
	if (evt.type == SDL_KEYDOWN){
		if (evt.key.keysym.sym == SDLK_w){
			left_move_dir = 1;
		}
		else if (evt.key.keysym.sym == SDLK_s){
			left_move_dir = 2;
		}
		if (evt.key.keysym.sym == SDLK_UP){
			right_move_dir = 1;
		}
		else if (evt.key.keysym.sym == SDLK_DOWN){
			right_move_dir = 2;
		}
	}
	// stop moving on release
	if (evt.type == SDL_KEYUP && (evt.key.keysym.sym == SDLK_w || evt.key.keysym.sym == SDLK_s) ){
		left_move_dir = 0;
	}
	if (evt.type == SDL_KEYUP && (evt.key.keysym.sym == SDLK_UP || evt.key.keysym.sym == SDLK_DOWN) ){
		right_move_dir = 0;
	}

	// -- player shooting --
	if (evt.type == SDL_KEYDOWN){
		//left player
		if (evt.key.keysym.sym == SDLK_d){
			if(left_bullet_level >= 1){
				bullets.emplace_back( bullet(1, 20.0f+3*(4-left_bullet_level), 0.1f * (float)pow(2, left_bullet_level), left_bullet_level, left_paddle+left_bullet_offset) );
				left_bullet_level = 0;
				left_bullet_store_time = 0;
			}
		}
		//right player
		if (evt.key.keysym.sym == SDLK_LEFT){
			if(right_bullet_level >= 1){
				bullets.emplace_back( bullet(2, 20.0f+3*(4-right_bullet_level), 0.1f*(float)pow(2,right_bullet_level), right_bullet_level, right_paddle+right_bullet_offset) );
				right_bullet_level = 0;
				right_bullet_store_time = 0;
			}
		}
	}

	return false;
}

void PongMode::update(float elapsed) {

	static std::mt19937 mt; //mersenne twister pseudo-random number generator

	// update bullet store time & cacualte cur bullet level
	left_bullet_store_time += elapsed;
	right_bullet_store_time += elapsed;
	left_bullet_level = (int) floor( left_bullet_store_time / bullet_levelup_needed_time);
	left_bullet_level = std::min(left_bullet_level, 3);
	right_bullet_level = (int) floor( right_bullet_store_time / bullet_levelup_needed_time);
	right_bullet_level = std::min(right_bullet_level, 3);

	// left player move
	switch (left_move_dir)
	{
		case 1:
			left_paddle.y += paddle_move_speed * elapsed;
			break;
		case 2:
			left_paddle.y -= paddle_move_speed * elapsed;
			break;
	
		default:
			break;
	}
	// right player move
	switch (right_move_dir)
	{
		case 1:
			right_paddle.y += paddle_move_speed * elapsed;
			break;
		case 2:
			right_paddle.y -= paddle_move_speed * elapsed;
			break;
	
		default:
			break;
	}

	// update all bullets
	for (auto it = bullets.begin(); it != bullets.end(); it++){
		bullet & blt = *it;
		if(blt.m_shooter == 1)
			blt.m_pos.x += blt.m_speed * elapsed;
		else if(blt.m_shooter == 2)
			blt.m_pos.x -= blt.m_speed * elapsed;
		// delete bullets that are out of court:
		if (blt.m_pos.x < -court_radius.x || blt.m_pos.x > court_radius.x){
			bullets.erase(it);
		}
	} 

	//clamp paddles to court:
	right_paddle.y = std::max(right_paddle.y, -court_radius.y + paddle_radius.y);
	right_paddle.y = std::min(right_paddle.y,  court_radius.y - paddle_radius.y);

	left_paddle.y = std::max(left_paddle.y, -court_radius.y + paddle_radius.y);
	left_paddle.y = std::min(left_paddle.y,  court_radius.y - paddle_radius.y);

	//---- collision handling ----

	// signal "get hit" for a peroid of time for color change
	auto current_time = std::chrono::high_resolution_clock::now();
	static auto left_last_hit_time = current_time;
	static auto right_last_hit_time = current_time;
	float left_elapsed = std::chrono::duration< float >(current_time - left_last_hit_time).count();
	float right_elapsed = std::chrono::duration< float >(current_time - right_last_hit_time).count();
	if(left_elapsed > 0.1f)
		left_got_hit = false;
	if(right_elapsed > 0.1f)
		right_got_hit = false;

	// check collision for all bullets
	for (auto it = bullets.begin(); it != bullets.end(); it++){
		bullet & blt = *it;
		if(blt.m_shooter == 1){
			// on collision of right player
			if (  (blt.m_pos.x + blt.m_radius >= right_paddle.x - paddle_radius.x) &&
				  (blt.m_pos.y - blt.m_radius <= right_paddle.y + paddle_radius.y) &&
				  (blt.m_pos.y + blt.m_radius >= right_paddle.y - paddle_radius.y)
				){
				// do dmg
				rightHp = std::max(0,rightHp-bulletDmg[blt.m_bulletLevel]);
				// set hit-signal for color change
				right_got_hit = true;
				right_last_hit_time = std::chrono::high_resolution_clock::now();
				// delete bullet on collision
				bullets.erase(it);
			}
		}
		if(blt.m_shooter == 2){
			// on collision of left player
			if (  (blt.m_pos.x - blt.m_radius <= left_paddle.x + paddle_radius.x) &&
				  (blt.m_pos.y - blt.m_radius <= left_paddle.y + paddle_radius.y) &&
				  (blt.m_pos.y + blt.m_radius >= left_paddle.y - paddle_radius.y)
				){
				// do dmg
				leftHp = std::max(0,leftHp-bulletDmg[blt.m_bulletLevel]);
				// set hit-signal for color change
				left_got_hit = true;
				left_last_hit_time = std::chrono::high_resolution_clock::now();
				// delete bullet on collision
				bullets.erase(it);
			}
		}		
	} 

}

void PongMode::draw(glm::uvec2 const &drawable_size) {
	//some nice colors from the course web page:
	#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
	const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0x85c76d11);
	const glm::u8vec4 left_paddle_color = HEX_TO_U8VEC4(0xf29dfaff); 
	const glm::u8vec4 right_paddle_color = HEX_TO_U8VEC4(0xa1a5ffff); 
	const glm::u8vec4 mouth_color = HEX_TO_U8VEC4(0xcf5d4eff); 
	const glm::u8vec4 eye_color = HEX_TO_U8VEC4(0x403837ff); 
	const glm::u8vec4 eyebrow_color = HEX_TO_U8VEC4(0xfae7c8ff); 
	const glm::u8vec4 hit_color = HEX_TO_U8VEC4(0xff3e1cff); 
	const glm::u8vec4 fg_color = HEX_TO_U8VEC4(0xd9d36fff);
	const glm::u8vec4 shadow_color = HEX_TO_U8VEC4(0xf2ad94ff);
	const std::vector< glm::u8vec4 > bullet_color = {
		HEX_TO_U8VEC4(0x666666ff),
		HEX_TO_U8VEC4(0x95ddfcff),
		HEX_TO_U8VEC4(0x0593ffff),
		HEX_TO_U8VEC4(0x2d40cfff),
	};
	#undef HEX_TO_U8VEC4

	//other useful drawing constants:
	const float wall_radius = 0.05f;
	const float shadow_offset = 0.07f;
	const float padding = 0.14f; //padding between outside of walls and edge of window

	//---- compute vertices to draw ----

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector< Vertex > vertices;

	//inline helper function for rectangle drawing:
	auto draw_rectangle = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color) {
		//draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	// bullets
	for (bullet blt : bullets){
		draw_rectangle(blt.m_pos,glm::vec2(blt.m_radius,blt.m_radius),bullet_color[blt.m_bulletLevel]);
	}

	//walls:
	draw_rectangle(glm::vec2(-court_radius.x-wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2( court_radius.x+wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2( 0.0f,-court_radius.y-wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);
	draw_rectangle(glm::vec2( 0.0f, court_radius.y+wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);

	//paddles funct:
	auto draw_paddle = [&draw_rectangle,&mouth_color,&eye_color,&eyebrow_color]
		(glm::vec2 const &center, glm::vec2 const &bullet_offset, glm::vec2 const &radius, glm::u8vec4 const &color, int player) 
	{
		draw_rectangle(center, radius, color);
		if (player == 1){
			// "mouth"
			draw_rectangle(center+bullet_offset, glm::vec2(0.2,0.3), mouth_color);
			// "eye"
			draw_rectangle(center+glm::vec2(0.25,0.5), glm::vec2(0.15,0.15), eye_color);
			draw_rectangle(center+glm::vec2(0.15,0.7), glm::vec2(0.3,0.1), eyebrow_color);
		}
		else if (player == 2){
			// "mouth"
			draw_rectangle(center+bullet_offset, glm::vec2(0.2,0.3), mouth_color);
			// "eye"
			draw_rectangle(center+glm::vec2(-0.25,0.5), glm::vec2(0.15,0.15), eye_color);
			draw_rectangle(center+glm::vec2(-0.15,0.7), glm::vec2(0.3,0.1), eyebrow_color);
		}
	};
	// paddle actual 
	if(left_got_hit)
		draw_paddle(left_paddle,left_bullet_offset, paddle_radius, hit_color,1);
	else
		draw_paddle(left_paddle, left_bullet_offset, paddle_radius, left_paddle_color,1);
	if(right_got_hit)
		draw_paddle(right_paddle, right_bullet_offset, paddle_radius, hit_color,2);
	else
		draw_paddle(right_paddle, right_bullet_offset, paddle_radius, right_paddle_color,2);

	// bullet level bar
	// left
	draw_rectangle(glm::vec2(-court_radius.x + 0.2 + 0.6*left_bullet_level, court_radius.y + 0.5), 
		glm::vec2(0.35 + 0.6*left_bullet_level , 0.3), bullet_color[left_bullet_level]);
	// right
	draw_rectangle(glm::vec2(court_radius.x - 0.2 - 0.6*right_bullet_level, court_radius.y + 0.5), 
		glm::vec2(0.35 + 0.6*right_bullet_level , 0.3), bullet_color[right_bullet_level]);

	// hp bar
	// left
	draw_rectangle(glm::vec2(-court_radius.x -0.1 + 2.5 * leftHp/maxHp,  -court_radius.y - 0.5), 
		glm::vec2(2.5 *leftHp/maxHp , 0.3), hit_color);
	// right
	draw_rectangle(glm::vec2(court_radius.x +0.1 - 2.5 * rightHp/maxHp, -court_radius.y - 0.5), 
		glm::vec2(2.5 *rightHp/maxHp , 0.3), hit_color);


	//------ compute court-to-window transform ------

	//compute area that should be visible:
	glm::vec2 scene_min = glm::vec2(
		-court_radius.x - 2.0f * wall_radius - padding,
		-court_radius.y - 2.0f * wall_radius -0.8 - padding
	);
	glm::vec2 scene_max = glm::vec2(
		court_radius.x + 2.0f * wall_radius + padding,
		court_radius.y + 2.0f * wall_radius + 0.8 + padding
	);

	//compute window aspect ratio:
	float aspect = drawable_size.x / float(drawable_size.y);
	//we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

	//compute scale factor for court given that...
	float scale = std::min(
		(2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
		(2.0f) / (scene_max.y - scene_min.y) //... y must fit in [-1,1].
	);

	glm::vec2 center = 0.5f * (scene_max + scene_min);

	//build matrix that scales and translates appropriately:
	glm::mat4 court_to_clip = glm::mat4(
		glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
	);
	//NOTE: glm matrices are specified in *Column-Major* order,
	// so each line above is specifying a *column* of the matrix(!)

	//also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
	clip_to_court = glm::mat3x2(
		glm::vec2(aspect / scale, 0.0f),
		glm::vec2(0.0f, 1.0f / scale),
		glm::vec2(center.x, center.y)
	);

	//---- actual drawing ----

	//clear the color buffer:
	glClearColor(bg_color.r / 255.0f, bg_color.g / 255.0f, bg_color.b / 255.0f, bg_color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero so things will be drawn just with their colors:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, white_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);
	

	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.

}
