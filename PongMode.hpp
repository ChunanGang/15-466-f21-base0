#include "ColorTextureProgram.hpp"

#include "Mode.hpp"
#include "GL.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <list>
#include <chrono>
#include <math.h> 

/*
 * PongMode is a game mode that implements a single-player game of Pong.
 */

struct PongMode : Mode {
	PongMode();
	virtual ~PongMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----
	glm::vec2 court_radius = glm::vec2(7.0f, 5.0f);
	glm::vec2 paddle_radius = glm::vec2(0.5f, 1.0f);

	glm::vec2 left_paddle = glm::vec2(-court_radius.x + 0.5f, 0.0f);
	glm::vec2 right_paddle = glm::vec2( court_radius.x - 0.5f, 0.0f);
	glm::vec2 left_bullet_offset = glm::vec2( paddle_radius.x+0.2, -0.3);
	glm::vec2 right_bullet_offset = glm::vec2( -paddle_radius.x-0.2, -0.3);

	bool right_got_hit = false;
	bool left_got_hit = false;

	// player movement
	const float paddle_move_speed = 10.0f; 
	int left_move_dir = 0; // 0 for no move; 1,2,3,4 for up down left right
	int right_move_dir = 0; // 0 for no move; 1,2,3,4 for up down left right

	// player status
	int maxHp = 100;
	int leftHp = maxHp;
	int rightHp = maxHp;

	// bullets
	struct bullet{
		int m_shooter; // 1 for left, 2 for right
		float m_speed;
		float m_radius;
		int m_bulletLevel;
		glm::vec2 m_pos;
		bullet(int shooter, float speed, float radius, int bulletLevel,glm::vec2 pos):
			m_shooter(shooter),m_speed(speed),m_radius(radius), m_pos(pos), m_bulletLevel(bulletLevel){};
	};
	std::list<bullet> bullets;
	float left_bullet_store_time = 0;
	float right_bullet_store_time = 0;
	float bullet_levelup_needed_time = 1.5f;
	int left_bullet_level = 0;
	int right_bullet_level = 0;
	int bulletDmg[4] = {0,10,25,40};

	//----- opengl assets / helpers ------

	//draw functions will work on vectors of vertices, defined as follows:
	struct Vertex {
		Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) :
			Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};
	static_assert(sizeof(Vertex) == 4*3 + 1*4 + 4*2, "PongMode::Vertex should be packed");

	//Shader program that draws transformed, vertices tinted with vertex colors:
	ColorTextureProgram color_texture_program;

	//Buffer used to hold vertex data during drawing:
	GLuint vertex_buffer = 0;

	//Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
	GLuint vertex_buffer_for_color_texture_program = 0;

	//Solid white texture:
	GLuint white_tex = 0;

	//matrix that maps from clip coordinates to court-space coordinates:
	glm::mat3x2 clip_to_court = glm::mat3x2(1.0f);
	// computed in draw() as the inverse of OBJECT_TO_CLIP
	// (stored here so that the mouse handling code can use it to position the paddle)

};
