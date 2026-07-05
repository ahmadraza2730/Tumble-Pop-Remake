#include <iostream>
#include <fstream>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

using namespace sf;
using namespace std;

enum GameState {LEVEL1,LEVEL2,GAME_OVER};

void handlePlayerHit(int& player_lives,int& player_score,bool& player_dying,int& player_death_frame,Clock& player_death_timer,float& player_x,float& player_y,float respawn_x,float respawn_y,float& velocityY,bool& onGround,bool& player_invincible,Clock& invincibility_clock)
{
	player_lives--;
	player_score-=50;
	if(player_score<0) player_score=0;

	if(player_lives<=0)
	{
		player_score-=200;
		if(player_score<0) player_score=0;
		cout<<"GAME OVER!"<<endl;
		player_dying=true;
		player_death_frame=0;
		player_death_timer.restart();
	}
	else
	{
		player_x=respawn_x;
		player_y=respawn_y;
		velocityY=0;
		onGround=false;
		player_invincible=true;
		invincibility_clock.restart();
	}
}

void display_level(RenderWindow& window,char** lvl,Texture& bgTex,Sprite& bgSprite,Texture& blockTexture,Sprite& blockSprite,const int height,const int width,const int cell_size)
{
	window.draw(bgSprite);
	for(int i=0;i<height;i++)
	{
		for(int j=0;j<width;j++)
		{
			if(lvl[i][j]=='#')
			{
				blockSprite.setPosition(j*cell_size,i*cell_size);
				window.draw(blockSprite);
			}
		}
	}
}

void display_level2(RenderWindow& window,Texture& bgTex2,Sprite& bgSprite2,Texture& blockTexture,Sprite& blockSprite,char** lvl,const int height,const int width,const int cell_size)
{
	window.draw(bgSprite2);
	for(int i=0;i<height;i++)
	{
		for(int j=0;j<width;j++)
		{
			if(lvl[i][j]=='#')
			{
				blockSprite.setPosition(j*cell_size,i*cell_size);
				window.draw(blockSprite);
			}
		}
	}
}

void player_gravity(char** lvl,float& offset_y,float& velocityY,bool& onGround,const float& gravity,float& terminal_Velocity,float& player_x,float& player_y,const int cell_size,int& Pheight,int& Pwidth)
{
	offset_y=player_y;
	offset_y+=velocityY;

	char bottom_left_down=lvl[(int)(offset_y+Pheight)/cell_size][(int)(player_x)/cell_size];
	char bottom_right_down=lvl[(int)(offset_y+Pheight)/cell_size][(int)(player_x+Pwidth)/cell_size];
	char bottom_mid_down=lvl[(int)(offset_y+Pheight)/cell_size][(int)(player_x+Pwidth/2)/cell_size];

	if((bottom_left_down=='#' || bottom_mid_down=='#' || bottom_right_down=='#') && velocityY>=0)
	{
		onGround=true;
		velocityY=0;
		int ground_row=(int)(offset_y+Pheight)/cell_size;
		player_y=(ground_row*cell_size)-Pheight;
	}
	else
	{
		if(offset_y<cell_size)
		{
			player_y=cell_size;
			velocityY=0;
		}
		else
		{
			player_y=offset_y;
		}
		onGround=false;
	}

	if(!onGround)
	{
		velocityY+=gravity;
		if(velocityY>=terminal_Velocity)
			velocityY=terminal_Velocity;
	}
}

bool check_wall_collision(char** lvl,float new_x,float player_y,int PlayerWidth,int PlayerHeight,const int cell_size)
{
	int top_row=(int)(player_y)/cell_size;
	int bottom_row=(int)(player_y+PlayerHeight-1)/cell_size;
	int left_col=(int)(new_x)/cell_size;
	int right_col=(int)(new_x+PlayerWidth-1)/cell_size;

	if(left_col<0 || right_col>=18 || top_row<0 || bottom_row>=14)
		return true;
	if(lvl[bottom_row][left_col]=='#' || lvl[bottom_row][right_col]=='#')
		return true;
	return false;
}

bool is_in_vacuum_range(float player_x,float player_y,int player_width,int player_height,float enemy_x,float enemy_y,int enemy_width,int enemy_height,int vacuum_direction)
{
	const float vacuum_range=150.0f;
	const float vacuum_height=120.0f;

	float player_center_x=player_x+player_width/2;
	float player_center_y=player_y+player_height/2;
	float enemy_center_x=enemy_x+enemy_width/2;
	float enemy_center_y=enemy_y+enemy_height/2;

	if(vacuum_direction==1)
	{
		if(enemy_center_x<player_center_x) return false;
		float distance_x=enemy_center_x-player_center_x;
		if(distance_x>vacuum_range) return false;
		float distance_y=abs(enemy_center_y-player_center_y);
		if(distance_y>vacuum_height) return false;
	}
	else if(vacuum_direction==-1)
	{
		if(enemy_center_x>player_center_x) return false;
		float distance_x=player_center_x-enemy_center_x;
		if(distance_x>vacuum_range) return false;
		float distance_y=abs(enemy_center_y-player_center_y);
		if(distance_y>vacuum_height) return false;
	}
	else if(vacuum_direction==2)
	{
		if(enemy_center_y<player_center_y) return false;
		float distance_y=enemy_center_y-player_center_y;
		if(distance_y>vacuum_range) return false;
		float distance_x=abs(enemy_center_x-player_center_x);
		if(distance_x>vacuum_height) return false;
	}
	else if(vacuum_direction==-2)
	{
		if(enemy_center_y>player_center_y) return false;
		float distance_y=player_center_y-enemy_center_y;
		if(distance_y>vacuum_range) return false;
		float distance_x=abs(enemy_center_x-player_center_x);
		if(distance_x>vacuum_height) return false;
	}
	return true;
}

void update_walking_enemies(float enemy_x[],float enemy_y[],float enemy_speed[],int enemy_direction[],Clock enemy_pause_clock[],bool enemy_paused[],float enemy_pause_duration[],Sprite enemySprites[],Texture& enemyLeftTex,Texture& enemyRightTex,bool enemy_active[],int num_enemies,int enemy_width)
{
	for(int i=0;i<num_enemies;i++)
	{
		if(!enemy_active[i]) continue;
		if(enemy_paused[i])
		{
			if(enemy_pause_clock[i].getElapsedTime().asSeconds()>=enemy_pause_duration[i])
			{
				enemy_paused[i]=false;
				enemy_direction[i]*=-1;
				enemySprites[i].setTexture(enemy_direction[i]>0?enemyRightTex:enemyLeftTex);
			}
		}
		else
		{
			enemy_x[i]+=enemy_speed[i]*enemy_direction[i];

			float left_bound,right_bound;

			if(i==0) { left_bound=3*64; right_bound=6*64; }
			else if(i==1) { left_bound=11*64; right_bound=14*64; }
			else if(i==2) { left_bound=3*64; right_bound=5*64; }
			else if(i==3) { left_bound=12*64; right_bound=14*64; }
			else if(i==4) { left_bound=15*64; right_bound=17*60; }
			else if(i==5) { left_bound=0*64; right_bound=2*50; }
			else if(i==6) { left_bound=11*64; right_bound=17*60; }
			else { left_bound=3*64; right_bound=7*64; }

			if(enemy_x[i]<=left_bound || enemy_x[i]>=right_bound-enemy_width)
			{
				enemy_x[i]=enemy_x[i]<=left_bound?left_bound:right_bound-enemy_width;
				enemy_paused[i]=true;
				enemy_pause_clock[i].restart();
				enemy_pause_duration[i]=0.5f+static_cast<float>(rand()%15)/10.0f;
			}
		}
	}
}

void update_skeletons(float skeleton_x[],float skeleton_y[],float skeleton_speed[],int skeleton_direction[],Clock skeleton_pause_clock[],bool skeleton_paused[],float skeleton_pause_duration[],float skeleton_velocityY[],bool skeleton_onGround[],char** lvl,int cell_size,bool skeleton_active[],int num_skeletons)
{
	const float skeleton_gravity=1.0f;
	const float skeleton_jump_strength=-16.0f;
	const int skeleton_width=64,skeleton_height=104;

	for(int i=0;i<num_skeletons;i++)
	{
		if(!skeleton_active[i]) continue;
		if(!skeleton_onGround[i])
		{
			skeleton_velocityY[i]+=skeleton_gravity;
			if(skeleton_velocityY[i]>20.0f) skeleton_velocityY[i]=20.0f;
		}

		skeleton_y[i]+=skeleton_velocityY[i];
		int bottom_row=(int)(skeleton_y[i]+skeleton_height)/cell_size;
		int left_col=(int)(skeleton_x[i])/cell_size;
		int right_col=(int)(skeleton_x[i]+skeleton_width-1)/cell_size;
		int mid_col=(int)(skeleton_x[i]+skeleton_width/2)/cell_size;

		if(bottom_row<14 && (lvl[bottom_row][left_col]=='#' || lvl[bottom_row][right_col]=='#' || lvl[bottom_row][mid_col]=='#') && skeleton_velocityY[i]>=0)
		{
			skeleton_onGround[i]=true;
			skeleton_velocityY[i]=0;
			skeleton_y[i]=(bottom_row*cell_size)-skeleton_height;
		}
		else
		{
			if(skeleton_y[i]<cell_size)
			{
				skeleton_y[i]=cell_size;
				skeleton_velocityY[i]=0;
			}
			skeleton_onGround[i]=false;
		}

		if(skeleton_paused[i])
		{
			if(skeleton_pause_clock[i].getElapsedTime().asSeconds()>=skeleton_pause_duration[i])
			{
				skeleton_paused[i]=false;
				int action=rand()%4;

				if(action==0)
				{
					skeleton_direction[i]*=-1;
				}
				else if((action==1 || action==2) && skeleton_onGround[i])
				{
					if(action==2) skeleton_direction[i]*=-1;

					int head_row=(int)(skeleton_y[i])/cell_size;
					int row_1_above_head=head_row-1;
					int row_2_above_head=head_row-2;
					int left_col=(int)(skeleton_x[i])/cell_size;
					int right_col=(int)(skeleton_x[i]+skeleton_width)/cell_size;
					int mid_col=(int)(skeleton_x[i]+skeleton_width/2)/cell_size;

					bool platform_1_exists=false;
					bool platform_2_exists=false;

					if(row_1_above_head>=0 && row_1_above_head<14)
					{
						if(lvl[row_1_above_head][left_col]=='#' || lvl[row_1_above_head][mid_col]=='#' || lvl[row_1_above_head][right_col]=='#')
							platform_1_exists=true;
					}

					if(row_2_above_head>=0 && row_2_above_head<14)
					{
						if(lvl[row_2_above_head][left_col]=='#' || lvl[row_2_above_head][mid_col]=='#' || lvl[row_2_above_head][right_col]=='#')
							platform_2_exists=true;
					}

					if(!(platform_1_exists && platform_2_exists))
					{
						skeleton_velocityY[i]=skeleton_jump_strength;
						skeleton_onGround[i]=false;
					}
					else
					{
						skeleton_velocityY[i]=-5;
						skeleton_onGround[i]=false;
					}
				}
			}
		}
		else if(skeleton_onGround[i])
		{
			float next_x=skeleton_x[i]+skeleton_speed[i]*skeleton_direction[i];
			int current_row=(int)(skeleton_y[i]+skeleton_height/2)/cell_size;
			int next_col_front=skeleton_direction[i]>0?(int)(next_x+skeleton_width)/cell_size:(int)(next_x)/cell_size;

			bool wall_ahead=false;
			bool edge_ahead=false;

			if(next_col_front>=0 && next_col_front<18 && current_row>=0 && current_row<14)
			{
				wall_ahead=(lvl[current_row][next_col_front]=='#');
				int ground_check_row=current_row+1;
				if(ground_check_row<14)
					edge_ahead=(lvl[ground_check_row][next_col_front]!='#');
			}

			if(wall_ahead || edge_ahead || next_x<=0 || next_x>=(18*64-skeleton_width))
			{
				skeleton_paused[i]=true;
				skeleton_pause_clock[i].restart();
				skeleton_pause_duration[i]=0.3f+static_cast<float>(rand()%10)/10.0f;
			}
			else
			{
				skeleton_x[i]=next_x;
				if(rand()%200==0)
				{
					skeleton_paused[i]=true;
					skeleton_pause_clock[i].restart();
					skeleton_pause_duration[i]=0.5f+static_cast<float>(rand()%15)/10.0f;
				}
			}
		}
	}
}

void update_projectiles(float proj_x[],float proj_y[],float proj_vx[],float proj_vy[],char proj_type[],bool proj_active[],bool proj_stunned[],bool proj_rolling[],int proj_roll_dir[],float proj_roll_speed[],Clock proj_stun_timer[],Sprite proj_sprites[],Texture& projBallTex,char** lvl,int cell_size,int& proj_count,int proj_dir[],int max_projectiles)
{
	for(int i=proj_count-1;i>=0;i--)
	{
		if(!proj_active[i]) continue;

		Sprite& proj=proj_sprites[i];

		if(proj_stunned[i] && proj_stun_timer[i].getElapsedTime().asSeconds()>=2.0f)
		{
			proj_stunned[i]=false;
			proj_rolling[i]=true;
			proj_vy[i]=0;
			proj_vx[i]=0;
		}

		if(proj_rolling[i])
		{
			if(proj_dir[i]==1 || proj_dir[i]==-1)
			{
				proj_x[i]+=proj_roll_speed[i]*proj_roll_dir[i];
				int bottom_row=(int)(proj_y[i]+52)/cell_size;
				int col=(int)(proj_x[i]+32)/cell_size;

				if(bottom_row>=13 || (bottom_row>=0 && bottom_row<14 && col>=0 && col<18 && lvl[bottom_row+1][col]!='#'))
				{
					proj_vy[i]+=1.0f;
					if(proj_vy[i]>15.0f) proj_vy[i]=15.0f;
					proj_y[i]+=proj_vy[i];
				}
				else if(bottom_row<14 && col>=0 && col<18 && lvl[bottom_row][col]=='#')
				{
					proj_y[i]=(bottom_row*cell_size)-52;
					proj_vy[i]=0;
				}

				int front_col=proj_roll_dir[i]>0?(int)(proj_x[i]+64)/cell_size:(int)(proj_x[i])/cell_size;
				int mid_row=(int)(proj_y[i]+26)/cell_size;

				if(front_col<0 || front_col>=18 || (mid_row>=0 && mid_row<14 && lvl[mid_row][front_col]=='#'))
					proj_roll_dir[i]*=-1;
			}
			else if(proj_dir[i]==2 || proj_dir[i]==-2)
			{
				proj_y[i]+=proj_roll_speed[i]*(proj_dir[i]==2?1:-1);
				int row=(int)(proj_y[i]+26)/cell_size;
				int left_col=(int)(proj_x[i])/cell_size;

				if(proj_dir[i]==2)
				{
					if(row>=13 || (row>=0 && row<14 && left_col>=0 && left_col<18 && lvl[row+1][left_col]!='#'))
					{
						proj_vx[i]+=0.5f;
						if(proj_vx[i]>8.0f) proj_vx[i]=8.0f;
						proj_x[i]+=proj_vx[i];
					}
					else if(row<14 && left_col>=0 && left_col<18 && lvl[row][left_col]=='#')
					{
						proj_y[i]=(row*cell_size)-52;
						proj_vx[i]=0;
					}
				}
				else
				{
					if(row<=0 || (row>=0 && row<14 && left_col>=0 && left_col<18 && lvl[row-1][left_col]!='#'))
					{
						proj_vx[i]-=0.5f;
						if(proj_vx[i]<-8.0f) proj_vx[i]=-8.0f;
						proj_x[i]+=proj_vx[i];
					}
				}
			}

			if(proj_x[i]<-100 || proj_x[i]>1200 || proj_y[i]>1000 || proj_y[i]<-100)
				proj_active[i]=false;
		}
		else if(!proj_stunned[i])
		{
			proj_x[i]+=proj_vx[i];
			proj_y[i]+=proj_vy[i];

			if(abs(proj_vy[i])>0.1f && (proj_dir[i]==1 || proj_dir[i]==-1))
				proj_vy[i]+=0.8f;

			int row=(int)(proj_y[i]+26)/cell_size;
			int col=(int)(proj_x[i]+32)/cell_size;

			if(row>=0 && row<14 && col>=0 && col<18)
			{
				if(lvl[row][col]=='#')
				{
					proj_stunned[i]=true;
					proj_rolling[i]=false;
					proj_vx[i]=0;
					proj_vy[i]=0;
					proj_stun_timer[i].restart();
					proj_y[i]=(row*cell_size)-52;
				}
			}

			if(proj_x[i]<-100 || proj_x[i]>1200 || proj_y[i]>1000 || proj_y[i]<-100)
				proj_active[i]=false;
		}

		proj.setPosition(proj_x[i],proj_y[i]);
	}
}

void check_projectile_enemy_collision(float proj_x[],float proj_y[],char proj_type[],bool proj_active[],float ghost_x[],float ghost_y[],bool ghost_active[],int num_ghosts,float chelnov_x[],float chelnov_y[],bool chelnov_active[],int num_chelnovs,float invisibleman_x[],float invisibleman_y[],bool invisibleman_active[],int num_invisiblemen,float skeleton_x[],float skeleton_y[],bool skeleton_active[],int num_skeletons,float death_x[],float death_y[],bool death_active[],Clock death_timer[],int death_frame[],char death_type[],int& player_score,int& kills_in_window,Clock& multikill_timer,int max_projectiles,int max_death_anims)
{
	for(int i=0;i<max_projectiles;i++)
	{
		if(!proj_active[i]) continue;
		int proj_width=64,proj_height=64;

		for(int g=0;g<num_ghosts;g++)
		{
			if(!ghost_active[g]) continue;
			if(proj_x[i]<ghost_x[g]+126 && proj_x[i]+proj_width>ghost_x[g] && proj_y[i]<ghost_y[g]+120 && proj_y[i]+proj_height>ghost_y[g])
			{
				ghost_active[g]=false;
				proj_active[i]=false;
				player_score+=100;
				kills_in_window++;
				multikill_timer.restart();

				for(int d=0;d<max_death_anims;d++)
				{
					if(!death_active[d])
					{
						death_active[d]=true;
						death_x[d]=ghost_x[g];
						death_y[d]=ghost_y[g];
						death_frame[d]=0;
						death_type[d]='G';
						death_timer[d].restart();
						break;
					}
				}
				break;
			}
		}

		if(!proj_active[i]) continue;

		for(int c=0;c<num_chelnovs;c++)
		{
			if(!chelnov_active[c]) continue;
			if(proj_x[i]<chelnov_x[c]+126 && proj_x[i]+proj_width>chelnov_x[c] && proj_y[i]<chelnov_y[c]+120 && proj_y[i]+proj_height>chelnov_y[c])
			{
				chelnov_active[c]=false;
				proj_active[i]=false;
				player_score+=120;
				kills_in_window++;
				multikill_timer.restart();

				for(int d=0;d<max_death_anims;d++)
				{
					if(!death_active[d])
					{
						death_active[d]=true;
						death_x[d]=chelnov_x[c];
						death_y[d]=chelnov_y[c];
						death_frame[d]=0;
						death_type[d]='C';
						death_timer[d].restart();
						break;
					}
				}
				break;
			}
		}

		if(!proj_active[i]) continue;

		for(int im=0;im<num_invisiblemen;im++)
		{
			if(!invisibleman_active[im]) continue;
			if(proj_x[i]<invisibleman_x[im]+64 && proj_x[i]+proj_width>invisibleman_x[im] && proj_y[i]<invisibleman_y[im]+104 && proj_y[i]+proj_height>invisibleman_y[im])
			{
				invisibleman_active[im]=false;
				proj_active[i]=false;
				player_score+=150;
				kills_in_window++;
				multikill_timer.restart();

				for(int d=0;d<max_death_anims;d++)
				{
					if(!death_active[d])
					{
						death_active[d]=true;
						death_x[d]=invisibleman_x[im];
						death_y[d]=invisibleman_y[im];
						death_frame[d]=0;
						death_type[d]='I';
						death_timer[d].restart();
						break;
					}
				}
				break;
			}
		}

		if(!proj_active[i]) continue;

		for(int s=0;s<num_skeletons;s++)
		{
			if(!skeleton_active[s]) continue;
			if(proj_x[i]<skeleton_x[s]+91 && proj_x[i]+proj_width>skeleton_x[s] && proj_y[i]<skeleton_y[s]+107 && proj_y[i]+proj_height>skeleton_y[s])
			{
				skeleton_active[s]=false;
				proj_active[i]=false;
				player_score+=150;
				kills_in_window++;
				multikill_timer.restart();

				for(int d=0;d<max_death_anims;d++)
				{
					if(!death_active[d])
					{
						death_active[d]=true;
						death_x[d]=skeleton_x[s];
						death_y[d]=skeleton_y[s];
						death_frame[d]=0;
						death_type[d]='S';
						death_timer[d].restart();
						break;
					}
				}
				break;
			}
		}
	}
}

bool check_player_enemy_collision(float player_x,float player_y,int player_width,int player_height,float enemy_x,float enemy_y,int enemy_width,int enemy_height)
{
	return(player_x<enemy_x+enemy_width && player_x+player_width>enemy_x && player_y<enemy_y+enemy_height && player_y+player_height>enemy_y);
}

void spawn_powerup(float powerup_x[],float powerup_y[],char powerup_type[],bool powerup_active[],int max_powerups,char** lvl,int height,int width,int cell_size)
{
	int slot=-1;
	for(int i=0;i<max_powerups;i++)
	{
		if(!powerup_active[i])
		{
			slot=i;
			break;
		}
	}

	if(slot==-1) return;

	int attempts=0;
	bool valid_position=false;
	int spawn_row,spawn_col;

	while(!valid_position && attempts<100)
	{
		spawn_row=2+rand()%(height-3);
		spawn_col=rand()%width;

		if(lvl[spawn_row][spawn_col]!='#')
		{
			if(spawn_row+1<height && lvl[spawn_row+1][spawn_col]=='#')
			{
				bool space_above=true;
				if(spawn_row-1>=0 && lvl[spawn_row-1][spawn_col]=='#')
					space_above=false;
				if(spawn_row-2>=0 && lvl[spawn_row-2][spawn_col]=='#')
					space_above=false;

				if(space_above)
					valid_position=true;
			}
		}
		attempts++;
	}

	if(valid_position)
	{
		powerup_active[slot]=true;
		powerup_x[slot]=spawn_col*cell_size;
		powerup_y[slot]=spawn_row*cell_size;
		powerup_type[slot]=(rand()%2==0)?'S':'H';
	}
}

void check_powerup_collision(float player_x,float player_y,int player_width,int player_height,float powerup_x[],float powerup_y[],char powerup_type[],bool powerup_active[],int max_powerups,int powerup_size,int& player_lives,bool& speed_boost_active,Clock& speed_boost_timer,float& speed,float base_speed)
{
	for(int i=0;i<max_powerups;i++)
	{
		if(!powerup_active[i]) continue;

		if(player_x<powerup_x[i]+powerup_size && player_x+player_width>powerup_x[i] && player_y<powerup_y[i]+powerup_size && player_y+player_height>powerup_y[i])
		{
			if(powerup_type[i]=='S')
			{
				speed_boost_active=true;
				speed_boost_timer.restart();
				speed=base_speed*2.0f;
			}
			else if(powerup_type[i]=='H')
			{
				player_lives++;
			}
			powerup_active[i]=false;
		}
	}
}

void init_level2(char** lvl,float& player_x,float& player_y,float& respawn_x,float& respawn_y,float ghost_x[],float ghost_y[],bool ghost_active[],int& num_ghosts,float chelnov_x[],float chelnov_y[],bool chelnov_active[],int& num_chelnovs,float invisibleman_x[],float invisibleman_y[],bool invisibleman_active[],int& num_invisiblemen,float skeleton_x[],float skeleton_y[],bool skeleton_active[],int& num_skeletons)
{
	for(int i=0;i<14;i++) {
		for(int j=0;j<18;j++) {
			lvl[i][j]=' ';
		}
	}

	for(int i=0;i<18;i++) {
		lvl[13][i]='#';
		lvl[12][i]='#';
	}

	for(int i=0;i<3;i++){
		lvl[2][i]='#';
	}
	for(int i=6;i<15;i++){
		lvl[2][i]='#';
	}

	lvl[3][2]='#';
	lvl[3][3]='#';
	lvl[4][3]='#';
	lvl[4][4]='#';
	lvl[5][4]='#';
	lvl[5][5]='#';
	lvl[6][5]='#';
	lvl[6][6]='#';
	lvl[7][6]='#';
	lvl[7][7]='#';
	lvl[8][7]='#';
	lvl[8][8]='#';
	lvl[9][8]='#';
	lvl[9][9]='#';
	lvl[9][10]='#';
	lvl[9][11]='#';
	lvl[10][9]='#';
	lvl[10][10]='#';
	lvl[10][11]='#';
	lvl[10][12]='#';
	lvl[10][13]='#';
	lvl[10][14]='#';

	for(int i=0;i<18;i++){
		if((i<=7 && i>=1)||i==13||i==14||i==8){
			continue;
		}
		else
			lvl[4][i]='#';
	}

	for(int i=0;i<18;i++){
		if((i>=11 && i<=15) || i==0 || i==1){
			lvl[6][i]='#';
		}
		else
			continue;
	}
	for(int i=0;i<18;i++){
		if((i>=14 && i<=17) || (i>=0 && i<=3)){
			lvl[8][i]='#';
		}
		else
			continue;
	}
	for(int i=0;i<18;i++){
		if(i>=0 && i<=5){
			lvl[10][i]='#';
		}
		else
			continue;
	}

	for(int i=0;i<18;i++){
		lvl[13][i]='#';
	}

	player_x=7*64;
	player_y=11*64;
	respawn_x=player_x;
	respawn_y=player_y;

	num_ghosts=4;
	ghost_x[0]=3*64; ghost_y[0]=3*64;
	ghost_x[1]=14*64; ghost_y[1]=3*64;
	ghost_x[2]=8*64; ghost_y[2]=5*64;
	ghost_x[3]=4*64; ghost_y[3]=9*64;

	for(int i=0;i<12;i++) ghost_active[i]=(i<num_ghosts);

	num_chelnovs=4;
	chelnov_x[0]=5*64; chelnov_y[0]=2*64;
	chelnov_x[1]=12*64; chelnov_y[1]=2*64;
	chelnov_x[2]=6*64; chelnov_y[2]=7*64;
	chelnov_x[3]=11*64; chelnov_y[3]=10*64;

	for(int i=0;i<12;i++) chelnov_active[i]=(i<num_chelnovs);

	num_invisiblemen=3;
	invisibleman_x[0]=2*64; invisibleman_y[0]=12*64;
	invisibleman_x[1]=9*64; invisibleman_y[1]=5*64;
	invisibleman_x[2]=15*64; invisibleman_y[2]=12*64;

	for(int i=0;i<12;i++) invisibleman_active[i]=(i<num_invisiblemen);

	num_skeletons=9;
	skeleton_x[0]=7*64; skeleton_y[0]=12*64;
	skeleton_x[1]=10*64; skeleton_y[1]=12*64;
	skeleton_x[2]=13*64; skeleton_y[2]=12*64;
	skeleton_x[3]=4*64; skeleton_y[3]=8*64;
	skeleton_x[4]=14*64; skeleton_y[4]=7*64;
	skeleton_x[5]=2*64; skeleton_y[5]=6*64;
	skeleton_x[6]=16*64; skeleton_y[6]=9*64;
	skeleton_x[7]=12*64; skeleton_y[7]=6*64;
	skeleton_x[8]=5*64; skeleton_y[8]=4*64;

	for(int i=0;i<12;i++) skeleton_active[i]=(i<num_skeletons);
}

int main()
{
	int screen_x=1136;
	int screen_y=896;

	const int MAX_CAPTURED=3;
	const int MAX_PROJECTILES=20;
	const int MAX_DEATH_ANIMS=10;
	const int MAX_POWERUPS=5;

	GameState currentState=LEVEL1;

	RenderWindow window(VideoMode(1136,896),"Tumble-POP",Style::Resize);
	window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(60);

	const int cell_size=64;
	const int height=14;
	const int width=18;
	const int POWERUP_SIZE=48;
	char** lvl;

	Texture bgTex,blockTexture,PlayerTexture,runRightTexture,runLeftTexture;
	Texture playerUpTexture,playerDownTexture;
	Texture ghostTexture,ghostRightTex;
	Texture chelnovLeftTex,chelnovRightTex;
	Texture invisiblemanLeftTex,invisiblemanRightTex;
	Texture skeletonTexture;
	Texture vacuumLightRight,vacuumLightLeft,vacuumLightUp,vacuumLightDown;
	Texture ghostSuckTexture,skeletonSuckTexture;
	Texture ghostDeathTex,skeletonDeathTex,projBallTex;
	Texture heartTexture,playerFaceTexture;
	Texture bgTex2;
	Texture speedPowerupTex,healthPowerupTex;

	Sprite bgSprite,blockSprite,PlayerSprite;
	Sprite bgSprite2;
	Sprite vacuumLightSpriteRight,vacuumLightSpriteLeft,vacuumLightSpriteUp,vacuumLightSpriteDown;
	Sprite ghostSprites[12];
	Sprite chelnovSprites[12];
	Sprite invisiblemanSprites[12];
	Sprite skeletonSprites[12];
	Sprite projSprites[MAX_PROJECTILES];
	Sprite deathSprites[MAX_DEATH_ANIMS];
	Sprite suckSprite;
	Sprite heartSprites[3];
	Sprite playerFaceSprite;
	Sprite powerup_sprites[MAX_POWERUPS];

	bgTex.loadFromFile("Assets/Sprite/bg.png");
	bgSprite.setTexture(bgTex);
	bgSprite.setPosition(0,0);

	bgTex2.loadFromFile("Assets/Sprite/bg2.png");
	bgSprite2.setTexture(bgTex2);
	bgSprite2.setPosition(0,0);

	blockTexture.loadFromFile("Assets/Sprite/block1.png");
	blockSprite.setTexture(blockTexture);

	Music lvlMusic;
	lvlMusic.openFromFile("Assets/Sound/mus.ogg");
	lvlMusic.setVolume(20);
	lvlMusic.play();
	lvlMusic.setLoop(true);

	ghostTexture.loadFromFile("Assets/Sprite/ghostLeft.png");
	ghostRightTex.loadFromFile("Assets/Sprite/ghostRight.png");

	chelnovLeftTex.loadFromFile("Assets/Sprite/chelnovLeft.png");
	chelnovRightTex.loadFromFile("Assets/Sprite/chelnovRight.png");
	invisiblemanLeftTex.loadFromFile("Assets/Sprite/invisiblemanLeft.png");
	invisiblemanRightTex.loadFromFile("Assets/Sprite/invisiblemanRight.png");
	skeletonTexture.loadFromFile("Assets/Sprite/skeleton.png");

	float player_x=1*64;
	float player_y=11*64;
	float respawn_x=1*64;
	float respawn_y=11*64;
	float speed=5;
	float base_speed=5.0f;
	const float jumpStrength=-20;
	const float gravity=2;
	float velocityY=0;
	float terminal_Velocity=30;
	int PlayerHeight=102;
	int PlayerWidth=96;
	bool onGround=false;
	float offset_x=0;
	float offset_y=0;
	int player_facing=1;
	bool player_is_jumping=false;
	int vacuum_direction=0;

	int player_lives=3;
	bool player_invincible=false;
	Clock invincibility_clock;
	float invincibility_duration=2.0f;
	bool player_dying=false;
	Clock player_death_timer;
	int player_death_frame=0;

	int player_score=0;
	int kills_in_window=0;
	Clock multikill_timer;
	float multikill_window=2.0f;
	bool multikill_awarded=false;

	bool vacuum_burst_used=false;
	bool speed_boost_active=false;
	Clock speed_boost_timer;
	const float SPEED_BOOST_DURATION=7.0f;

	char captured_type[MAX_CAPTURED];
	int captured_count=0;
	bool isVacuuming=false;
	Clock vacuumClock;
	Clock vacuumLightClock;
	const float VACUUM_DURATION=0.1f;
	int vacuumLightFrame=0;

	bool show_suck_anim=false;
	Clock suckAnimClock;
	float suck_anim_x=0;
	float suck_anim_y=0;
	char suck_anim_type=' ';

	float proj_x[MAX_PROJECTILES],proj_y[MAX_PROJECTILES];
	float proj_vx[MAX_PROJECTILES],proj_vy[MAX_PROJECTILES];
	char proj_type[MAX_PROJECTILES];
	bool proj_active[MAX_PROJECTILES];
	bool proj_stunned[MAX_PROJECTILES];
	bool proj_rolling[MAX_PROJECTILES];
	int proj_roll_dir[MAX_PROJECTILES];
	float proj_roll_speed[MAX_PROJECTILES];
	int proj_dir[MAX_PROJECTILES];
	Clock proj_stun_timer[MAX_PROJECTILES];
	int proj_count=0;
	Clock proj_ball_clock[MAX_PROJECTILES];
	int proj_ball_frame[MAX_PROJECTILES];

	float death_x[MAX_DEATH_ANIMS];
	float death_y[MAX_DEATH_ANIMS];
	bool death_active[MAX_DEATH_ANIMS];
	Clock death_timer[MAX_DEATH_ANIMS];
	int death_frame[MAX_DEATH_ANIMS];
	char death_type[MAX_DEATH_ANIMS];

	float powerup_x[MAX_POWERUPS];
	float powerup_y[MAX_POWERUPS];
	char powerup_type[MAX_POWERUPS];
	bool powerup_active[MAX_POWERUPS];
	Clock powerup_spawn_clock;
	const float POWERUP_SPAWN_INTERVAL=10.0f;

	PlayerTexture.loadFromFile("Assets/Sprite/player.png");
	runRightTexture.loadFromFile("Assets/Sprite/playerright.png");
	runLeftTexture.loadFromFile("Assets/Sprite/playerleft.png");
	playerUpTexture.loadFromFile("Assets/Sprite/playerup.png");
	playerDownTexture.loadFromFile("Assets/Sprite/playerdown.png");
	PlayerSprite.setTexture(PlayerTexture);
	PlayerSprite.setScale(3,3);
	PlayerSprite.setPosition(0,120);

	vacuumLightRight.loadFromFile("Assets/Sprite/vacuumlightright.png");
	vacuumLightLeft.loadFromFile("Assets/Sprite/vacuumlightleft.png");
	vacuumLightUp.loadFromFile("Assets/Sprite/vacuumlightup.png");
	vacuumLightDown.loadFromFile("Assets/Sprite/vacuumlightdown.png");

	vacuumLightSpriteRight.setTexture(vacuumLightRight);
	vacuumLightSpriteRight.setScale(2,2);
	vacuumLightSpriteLeft.setTexture(vacuumLightLeft);
	vacuumLightSpriteLeft.setScale(2,2);
	vacuumLightSpriteUp.setTexture(vacuumLightUp);
	vacuumLightSpriteUp.setScale(2,2);
	vacuumLightSpriteDown.setTexture(vacuumLightDown);
	vacuumLightSpriteDown.setScale(2,2);

	ghostSuckTexture.loadFromFile("Assets/Sprite/ghostsuck.png");
	skeletonSuckTexture.loadFromFile("Assets/Sprite/skeletonsuck.png");
	ghostDeathTex.loadFromFile("Assets/Sprite/ghostdeath.png");
	skeletonDeathTex.loadFromFile("Assets/Sprite/skeletondeath.png");
	projBallTex.loadFromFile("Assets/Sprite/ballprojectile.png");

	heartTexture.loadFromFile("Assets/Sprite/heart.png");
	playerFaceTexture.loadFromFile("Assets/Sprite/playerface.png");
	speedPowerupTex.loadFromFile("Assets/Sprite/speed.png");
	healthPowerupTex.loadFromFile("Assets/Sprite/health.png");

	playerFaceSprite.setTexture(playerFaceTexture);
	playerFaceSprite.setScale(2,2);
	playerFaceSprite.setPosition(20,10);

	for(int i=0;i<3;i++)
	{
		heartSprites[i].setTexture(heartTexture);
		heartSprites[i].setScale(1.5,1.5);
		heartSprites[i].setPosition(100+(i*40),20);
	}

	for(int i=0;i<MAX_DEATH_ANIMS;i++)
	{
		death_active[i]=false;
		death_frame[i]=0;
		deathSprites[i].setScale(2,2);
	}

	for(int i=0;i<MAX_POWERUPS;i++)
	{
		powerup_active[i]=false;
		powerup_sprites[i].setScale(0.3,0.3);
	}

	powerup_spawn_clock.restart();

	Font gameFont;
	if(!gameFont.loadFromFile("Assets/Font/font.otf"))
	{
		cout<<"ERROR: Font 'font.otf' failed to load!"<<endl;
	}

	Text scoreText;
	scoreText.setFont(gameFont);
	scoreText.setCharacterSize(36);
	scoreText.setFillColor(Color::Yellow);
	scoreText.setOutlineColor(Color::Black);
	scoreText.setOutlineThickness(2);
	scoreText.setPosition(20,100);
	scoreText.setString("SCORE: 0");

	Text levelText;
	levelText.setFont(gameFont);
	levelText.setCharacterSize(36);
	levelText.setFillColor(Color::Green);
	levelText.setOutlineColor(Color::Black);
	levelText.setOutlineThickness(2);
	levelText.setPosition(800,100);
	levelText.setString("LEVEL: 1");

	Text speedBoostText;
	speedBoostText.setFont(gameFont);
	speedBoostText.setCharacterSize(32);
	speedBoostText.setFillColor(Color::Cyan);
	speedBoostText.setOutlineColor(Color::Blue);
	speedBoostText.setOutlineThickness(2);
	speedBoostText.setPosition(400,10);

	int num_ghosts=8;
	int num_chelnovs=0;
	int num_invisiblemen=0;
	int num_skeletons=4;

	float ghost_x[12],ghost_y[12];
	float ghost_speed[12];
	int ghost_direction[12];
	Clock ghost_pause_clock[12];
	bool ghost_paused[12];
	float ghost_pause_duration[12];
	bool ghost_active[12]={false};

	float chelnov_x[12],chelnov_y[12];
	float chelnov_speed[12];
	int chelnov_direction[12];
	Clock chelnov_pause_clock[12];
	bool chelnov_paused[12];
	float chelnov_pause_duration[12];
	bool chelnov_active[12]={false};

	float invisibleman_x[12],invisibleman_y[12];
	float invisibleman_speed[12];
	int invisibleman_direction[12];
	Clock invisibleman_pause_clock[12];
	bool invisibleman_paused[12];
	float invisibleman_pause_duration[12];
	float invisibleman_velocityY[12];
	bool invisibleman_onGround[12];
	bool invisibleman_active[12]={false};

	float skeleton_x[12],skeleton_y[12];
	float skeleton_speed[12];
	int skeleton_direction[12];
	Clock skeleton_pause_clock[12];
	bool skeleton_paused[12];
	float skeleton_pause_duration[12];
	float skeleton_velocityY[12];
	bool skeleton_onGround[12];
	bool skeleton_active[12]={false};

	for(int i=0;i<12;i++)
	{
		ghostSprites[i].setTexture(ghostTexture);
		ghostSprites[i].setScale(2,2);
		ghost_speed[i]=0.8f+static_cast<float>(rand()%8)/10.0f;
		ghost_direction[i]=(rand()%2)*2-1;
		ghost_paused[i]=false;
		ghost_pause_duration[i]=0;
		ghost_active[i]=false;
	}

	ghost_x[0]=320; ghost_y[0]=75;
	ghost_x[1]=768; ghost_y[1]=75;
	ghost_x[2]=256; ghost_y[2]=335;
	ghost_x[3]=832; ghost_y[3]=590;
	ghost_x[4]=1024; ghost_y[4]=205;
	ghost_x[5]=64; ghost_y[5]=205;
	ghost_x[6]=960; ghost_y[6]=715;
	ghost_x[7]=320; ghost_y[7]=590;

	for(int i=0;i<num_ghosts;i++)
		ghost_active[i]=true;

	for(int i=0;i<12;i++)
	{
		chelnovSprites[i].setTexture(chelnovLeftTex);
		chelnovSprites[i].setScale(2,2);
		chelnov_speed[i]=1.0f+static_cast<float>(rand()%8)/10.0f;
		chelnov_direction[i]=(rand()%2)*2-1;
		chelnov_paused[i]=false;
		chelnov_pause_duration[i]=0;
		chelnov_active[i]=false;
	}

	for(int i=0;i<12;i++)
	{
		invisiblemanSprites[i].setTexture(invisiblemanLeftTex);
		invisiblemanSprites[i].setScale(0.3,0.3);
		invisibleman_speed[i]=1.0f+static_cast<float>(rand()%10)/10.0f;
		invisibleman_direction[i]=(rand()%2)*2-1;
		invisibleman_paused[i]=false;
		invisibleman_pause_duration[i]=0;
		invisibleman_velocityY[i]=0;
		invisibleman_onGround[i]=false;
		invisibleman_active[i]=false;
	}

	for(int i=0;i<12;i++)
	{
		skeletonSprites[i].setTexture(skeletonTexture);
		skeletonSprites[i].setScale(0.3,0.3);
		skeleton_speed[i]=1.0f+static_cast<float>(rand()%10)/10.0f;
		skeleton_direction[i]=(rand()%2)*2-1;
		skeleton_paused[i]=false;
		skeleton_pause_duration[i]=0;
		skeleton_velocityY[i]=0;
		skeleton_onGround[i]=false;
		skeleton_active[i]=false;
	}

	for(int i=0;i<MAX_PROJECTILES;i++)
	{
		projSprites[i].setTexture(projBallTex);
		projSprites[i].setScale(2,2);
		proj_active[i]=false;
		proj_ball_frame[i]=0;
		proj_dir[i]=0;
	}

	lvl=new char*[height];
	for(int i=0;i<height;i++)
	{
		lvl[i]=new char[width];
	}

	for(int i=0;i<height;i++)
	{
		for(int j=0;j<width;j++)
		{
			lvl[i][j]=' ';
		}
	}

	for(int i=0;i<width;i++)
	{
		lvl[13][i]='#';
		lvl[0][i]='#';
	}

	for(int j=3;j<15;j++)
	{
		lvl[3][j]='#';
		lvl[11][j]='#';
	}

	lvl[4][8]='#';
	lvl[4][9]='#';

	for(int i=0;i<18;i++)
	{
		if(i==3 || i==4 || i==5 || i==6 || i==11 || i==12 || i==13 || i==14)
			continue;
		lvl[5][i]='#';
		lvl[9][i]='#';
	}

	for(int i=7;i<11;i++)
	{
		lvl[6][i]='#';
		lvl[8][i]='#';
	}

	for(int i=3;i<15;i++)
	{
		lvl[7][i]='#';
	}

	lvl[10][8]='#';
	lvl[10][9]='#';

	srand(time(0));

	for(int i=0;i<num_skeletons;i++)
	{
		int random_row,random_col;
		bool validPosition=false;
		int attempts=0;

		while(!validPosition && attempts<1000)
		{
			random_row=2+rand()%11;
			random_col=rand()%18;

			if(lvl[random_row][random_col]!='#' && 
			   lvl[random_row-1][random_col]!='#' && 
			   lvl[random_row+1][random_col]=='#')
			{
				bool positionTaken=false;
				for(int j=0;j<i;j++)
				{
					if(skeleton_x[j]==random_col*64 && skeleton_y[j]==random_row*64)
					{
						positionTaken=true;
						break;
					}
				}

				float skeleton_spawn_x=random_col*64;
				float skeleton_spawn_y=random_row*64;
				float distance_to_spawn=sqrt(pow(skeleton_spawn_x-respawn_x,2)+pow(skeleton_spawn_y-respawn_y,2));
				
				if(distance_to_spawn<100.0f)
					positionTaken=true;

				if(!positionTaken)
					validPosition=true;
			}
			attempts++;
		}

		skeleton_x[i]=random_col*64;
		skeleton_y[i]=random_row*64;
		skeleton_active[i]=true;
	}

	Event ev;
	Clock levelTransitionTimer;
	bool levelTransition=false;

	while(window.isOpen())
	{
		if(currentState==LEVEL1)
		{
			bool allEnemiesDefeated=true;

			for(int i=0;i<num_ghosts;i++)
			{
				if(ghost_active[i])
				{
					allEnemiesDefeated=false;
					break;
				}
			}

			if(allEnemiesDefeated)
			{
				for(int i=0;i<num_skeletons;i++)
				{
					if(skeleton_active[i])
					{
						allEnemiesDefeated=false;
						break;
					}
				}
			}

			bool playerSafe=onGround && !player_dying;

			if(allEnemiesDefeated && playerSafe && !levelTransition)
			{
				levelTransition=true;
				levelTransitionTimer.restart();
				levelText.setString("LEVEL COMPLETE!");
			}

			if(levelTransition && levelTransitionTimer.getElapsedTime().asSeconds()>=2.0f)
			{
				currentState=LEVEL2;
				levelTransition=false;
				player_lives=3;

				init_level2(lvl,player_x,player_y,respawn_x,respawn_y,ghost_x,ghost_y,ghost_active,num_ghosts,chelnov_x,chelnov_y,chelnov_active,num_chelnovs,invisibleman_x,invisibleman_y,invisibleman_active,num_invisiblemen,skeleton_x,skeleton_y,skeleton_active,num_skeletons);

				player_x=respawn_x;
				player_y=respawn_y;
				velocityY=0;
				onGround=false;
				player_invincible=false;
				player_dying=false;
				captured_count=0;

				for(int i=0;i<MAX_PROJECTILES;i++)
					proj_active[i]=false;
				proj_count=0;

				for(int i=0;i<MAX_DEATH_ANIMS;i++)
					death_active[i]=false;

				levelText.setString("LEVEL: 2");
				player_score+=1000;
			}
		}

		if(currentState==LEVEL2)
		{
			bool allEnemiesDefeated=true;

			for(int i=0;i<num_ghosts;i++)
				if(ghost_active[i]) { allEnemiesDefeated=false; break; }

			if(allEnemiesDefeated)
				for(int i=0;i<num_chelnovs;i++)
					if(chelnov_active[i]) { allEnemiesDefeated=false; break; }

			if(allEnemiesDefeated)
				for(int i=0;i<num_invisiblemen;i++)
					if(invisibleman_active[i]) { allEnemiesDefeated=false; break; }

			if(allEnemiesDefeated)
				for(int i=0;i<num_skeletons;i++)
					if(skeleton_active[i]) { allEnemiesDefeated=false; break; }

			bool playerSafe=onGround && !player_dying;

			if(allEnemiesDefeated && playerSafe && !levelTransition)
			{
				levelTransition=true;
				levelTransitionTimer.restart();
				levelText.setString("YOU WIN!");
			}

			if(levelTransition && levelTransitionTimer.getElapsedTime().asSeconds()>=3.0f)
			{
				window.close();
				cout<<"CONGRATULATIONS! YOU COMPLETED THE GAME!"<<endl;
				cout<<"FINAL SCORE: "<<player_score<<endl;
			}
		}

		if(!onGround && velocityY<0)
			player_is_jumping=true;
		else if(onGround)
			player_is_jumping=false;

		bool moving=false;

		while(window.pollEvent(ev))
		{
			if(ev.type==Event::Closed)
				window.close();

			if(ev.type==Event::KeyPressed && ev.key.code==Keyboard::X && captured_count>0)
			{
				int proj_slot=-1;
				for(int i=0;i<MAX_PROJECTILES;i++)
				{
					if(!proj_active[i])
					{
						proj_slot=i;
						break;
					}
				}

				if(proj_slot!=-1)
				{
					char enemy_type=captured_type[captured_count-1];
					captured_count--;

					proj_type[proj_slot]=enemy_type;
					proj_x[proj_slot]=player_x+PlayerWidth/2-32;
					proj_y[proj_slot]=player_y+PlayerHeight/2-26;
					proj_active[proj_slot]=true;
					proj_stunned[proj_slot]=false;
					proj_rolling[proj_slot]=false;
					proj_roll_speed[proj_slot]=4.0f;
					proj_roll_dir[proj_slot]=vacuum_direction==0?player_facing:vacuum_direction;
					proj_vx[proj_slot]=10.0f*(vacuum_direction==0?player_facing:(vacuum_direction==1 || vacuum_direction==-1?vacuum_direction:0));
					proj_vy[proj_slot]=10.0f*(vacuum_direction==2?1:(vacuum_direction==-2?-1:0));
					proj_dir[proj_slot]=vacuum_direction==0?player_facing:vacuum_direction;
					proj_ball_frame[proj_slot]=0;
					proj_ball_clock[proj_slot].restart();
					proj_count++;
				}
			}

			if(ev.type==Event::KeyPressed && ev.key.code==Keyboard::C && captured_count>0)
			{
				if(captured_count>=3)
					vacuum_burst_used=true;

				for(int c=captured_count-1;c>=0;c--)
				{
					int proj_slot=-1;
					for(int i=0;i<MAX_PROJECTILES;i++)
					{
						if(!proj_active[i])
						{
							proj_slot=i;
							break;
						}
					}

					if(proj_slot!=-1)
					{
						char enemy_type=captured_type[c];

						proj_type[proj_slot]=enemy_type;
						proj_x[proj_slot]=player_x+PlayerWidth/2-32;
						proj_y[proj_slot]=player_y+PlayerHeight/2-26;
						proj_active[proj_slot]=true;
						proj_stunned[proj_slot]=false;
						proj_rolling[proj_slot]=false;
						proj_roll_speed[proj_slot]=4.0f;
						proj_roll_dir[proj_slot]=vacuum_direction==0?player_facing:vacuum_direction;
						proj_vx[proj_slot]=10.0f*(vacuum_direction==0?player_facing:(vacuum_direction==1 || vacuum_direction==-1?vacuum_direction:0));
						proj_vy[proj_slot]=10.0f*(vacuum_direction==2?1:(vacuum_direction==-2?-1:0));
						proj_dir[proj_slot]=vacuum_direction==0?player_facing:vacuum_direction;
						proj_ball_frame[proj_slot]=0;
						proj_ball_clock[proj_slot].restart();
						proj_count++;
					}
				}
				captured_count=0;
			}
		}

		if(!levelTransition)
		{
			if(Keyboard::isKeyPressed(Keyboard::Up) && onGround)
			{
				int head_row=(int)(player_y)/cell_size;
				int row_1_above_head=head_row-1;
				int row_2_above_head=head_row-2;
				int left_col=(int)(player_x)/cell_size;
				int right_col=(int)(player_x+PlayerWidth)/cell_size;
				int mid_col=(int)(player_x+PlayerWidth/2)/cell_size;

				bool platform_1_exists=false;
				bool platform_2_exists=false;

				if(row_1_above_head>=0 && row_1_above_head<14)
				{
					if(lvl[row_1_above_head][left_col]=='#' || lvl[row_1_above_head][mid_col]=='#' || lvl[row_1_above_head][right_col]=='#')
						platform_1_exists=true;
				}

				if(row_2_above_head>=0 && row_2_above_head<14)
				{
					if(lvl[row_2_above_head][left_col]=='#' || lvl[row_2_above_head][mid_col]=='#' || lvl[row_2_above_head][right_col]=='#')
						platform_2_exists=true;
				}

				if(!(platform_1_exists && platform_2_exists))
					velocityY=jumpStrength;
				else
					velocityY=-8;

				onGround=false;
			}

			moving=false;

			if(Keyboard::isKeyPressed(Keyboard::Left))
			{
				float new_x=player_x-speed;
				if(!check_wall_collision(lvl,new_x,player_y,PlayerWidth,PlayerHeight,cell_size))
					player_x=new_x;
				moving=true;
				player_facing=-1;
				PlayerSprite.setTexture(runLeftTexture);
			}
			else if(Keyboard::isKeyPressed(Keyboard::Right))
			{
				float new_x=player_x+speed;
				if(!check_wall_collision(lvl,new_x,player_y,PlayerWidth,PlayerHeight,cell_size))
					player_x=new_x;
				moving=true;
				player_facing=1;
				PlayerSprite.setTexture(runRightTexture);
			}

			if(!moving && !isVacuuming)
			{
				PlayerSprite.setTexture(player_facing>0?runRightTexture:runLeftTexture);
			}

			if(Keyboard::isKeyPressed(Keyboard::Escape))
				window.close();

			if(Keyboard::isKeyPressed(Keyboard::A) && captured_count<MAX_CAPTURED && !player_dying)
			{
				if(!isVacuuming)
				{
					isVacuuming=true;
					vacuum_direction=-1;
					vacuumClock.restart();
					vacuumLightClock.restart();
				}

				if(vacuumLightClock.getElapsedTime().asMilliseconds()>100)
				{
					vacuumLightFrame=(vacuumLightFrame+1)%4;
					vacuumLightClock.restart();
				}

				if(vacuumClock.getElapsedTime().asSeconds()>=VACUUM_DURATION)
				{
					for(int i=0;i<num_ghosts && captured_count<MAX_CAPTURED;i++)
					{
						if(ghost_active[i] && is_in_vacuum_range(player_x,player_y,PlayerWidth,PlayerHeight,ghost_x[i],ghost_y[i],128,128,-1))
						{
							captured_type[captured_count]='G';
							ghost_active[i]=false;
							captured_count++;
							show_suck_anim=true;
							suck_anim_x=ghost_x[i];
							suck_anim_y=ghost_y[i];
							suck_anim_type='G';
							suckAnimClock.restart();
							player_score+=50;
							vacuumClock.restart();
							break;
						}
					}

					if(captured_count<MAX_CAPTURED)
					{
						for(int i=0;i<num_chelnovs && captured_count<MAX_CAPTURED;i++)
						{
							if(chelnov_active[i] && is_in_vacuum_range(player_x,player_y,PlayerWidth,PlayerHeight,chelnov_x[i],chelnov_y[i],128,128,-1))
							{
								captured_type[captured_count]='C';
								chelnov_active[i]=false;
								captured_count++;
								show_suck_anim=true;
								suck_anim_x=chelnov_x[i];
								suck_anim_y=chelnov_y[i];
								suck_anim_type='G';
								suckAnimClock.restart();
								player_score+=60;
								vacuumClock.restart();
								break;
							}
						}
					}

					if(captured_count<MAX_CAPTURED)
					{
						for(int i=0;i<num_invisiblemen && captured_count<MAX_CAPTURED;i++)
						{
							if(invisibleman_active[i] && is_in_vacuum_range(player_x,player_y,PlayerWidth,PlayerHeight,invisibleman_x[i],invisibleman_y[i],64,104,-1))
							{
								captured_type[captured_count]='I';
								invisibleman_active[i]=false;
								captured_count++;
								show_suck_anim=true;
								suck_anim_x=invisibleman_x[i];
								suck_anim_y=invisibleman_y[i];
								suck_anim_type='S';
								suckAnimClock.restart();
								player_score+=75;
								vacuumClock.restart();
								break;
							}
						}
					}

					if(captured_count<MAX_CAPTURED)
					{
						for(int i=0;i<num_skeletons && captured_count<MAX_CAPTURED;i++)
						{
							if(skeleton_active[i] && is_in_vacuum_range(player_x,player_y,PlayerWidth,PlayerHeight,skeleton_x[i],skeleton_y[i],64,104,-1))
							{
								captured_type[captured_count]='S';
								skeleton_active[i]=false;
								captured_count++;
								show_suck_anim=true;
								suck_anim_x=skeleton_x[i];
								suck_anim_y=skeleton_y[i];
								suck_anim_type='S';
								suckAnimClock.restart();
								player_score+=75;
								vacuumClock.restart();
								break;
							}
						}
					}
				}
			}
			else if(Keyboard::isKeyPressed(Keyboard::D) && captured_count<MAX_CAPTURED && !player_dying)
			{
				if(!isVacuuming)
				{
					isVacuuming=true;
					vacuum_direction=1;
					vacuumClock.restart();
					vacuumLightClock.restart();
				}

				if(vacuumLightClock.getElapsedTime().asMilliseconds()>100)
				{
					vacuumLightFrame=(vacuumLightFrame+1)%4;
					vacuumLightClock.restart();
				}

				if(vacuumClock.getElapsedTime().asSeconds()>=VACUUM_DURATION)
				{
					for(int i=0;i<num_ghosts && captured_count<MAX_CAPTURED;i++)
					{
						if(ghost_active[i] && is_in_vacuum_range(player_x,player_y,PlayerWidth,PlayerHeight,ghost_x[i],ghost_y[i],128,128,1))
						{
							captured_type[captured_count]='G';
							ghost_active[i]=false;
							captured_count++;
							show_suck_anim=true;
							suck_anim_x=ghost_x[i];
							suck_anim_y=ghost_y[i];
							suck_anim_type='G';
							suckAnimClock.restart();
							player_score+=50;
							vacuumClock.restart();
							break;
						}
					}

					if(captured_count<MAX_CAPTURED)
					{
						for(int i=0;i<num_chelnovs && captured_count<MAX_CAPTURED;i++)
						{
							if(chelnov_active[i] && is_in_vacuum_range(player_x,player_y,PlayerWidth,PlayerHeight,chelnov_x[i],chelnov_y[i],128,128,1))
							{
								captured_type[captured_count]='C';
								chelnov_active[i]=false;
								captured_count++;
								show_suck_anim=true;
								suck_anim_x=chelnov_x[i];
								suck_anim_y=chelnov_y[i];
								suck_anim_type='G';
								suckAnimClock.restart();
								player_score+=60;
								vacuumClock.restart();
								break;
							}
						}
					}

					if(captured_count<MAX_CAPTURED)
					{
						for(int i=0;i<num_invisiblemen && captured_count<MAX_CAPTURED;i++)
						{
							if(invisibleman_active[i] && is_in_vacuum_range(player_x,player_y,PlayerWidth,PlayerHeight,invisibleman_x[i],invisibleman_y[i],64,104,1))
							{
								captured_type[captured_count]='I';
								invisibleman_active[i]=false;
								captured_count++;
								show_suck_anim=true;
								suck_anim_x=invisibleman_x[i];
								suck_anim_y=invisibleman_y[i];
								suck_anim_type='S';
								suckAnimClock.restart();
								player_score+=75;
								vacuumClock.restart();
								break;
							}
						}
					}

					if(captured_count<MAX_CAPTURED)
					{
						for(int i=0;i<num_skeletons && captured_count<MAX_CAPTURED;i++)
						{
							if(skeleton_active[i] && is_in_vacuum_range(player_x,player_y,PlayerWidth,PlayerHeight,skeleton_x[i],skeleton_y[i],64,104,1))
							{
								captured_type[captured_count]='S';
								skeleton_active[i]=false;
								captured_count++;
								show_suck_anim=true;
								suck_anim_x=skeleton_x[i];
								suck_anim_y=skeleton_y[i];
								suck_anim_type='S';
								suckAnimClock.restart();
								player_score+=75;
								vacuumClock.restart();
								break;
							}
						}
					}
				}
			}
			else if(Keyboard::isKeyPressed(Keyboard::W) && captured_count<MAX_CAPTURED && !player_dying)
			{
				if(!isVacuuming)
				{
					isVacuuming=true;
					vacuum_direction=-2;
					vacuumClock.restart();
					vacuumLightClock.restart();
				}

				if(vacuumLightClock.getElapsedTime().asMilliseconds()>100)
				{
					vacuumLightFrame=(vacuumLightFrame+1)%4;
					vacuumLightClock.restart();
				}
			}
			else if(Keyboard::isKeyPressed(Keyboard::S) && captured_count<MAX_CAPTURED && !player_dying)
			{
				if(!isVacuuming)
				{
					isVacuuming=true;
					vacuum_direction=2;
					vacuumClock.restart();
					vacuumLightClock.restart();
				}

				if(vacuumLightClock.getElapsedTime().asMilliseconds()>100)
				{
					vacuumLightFrame=(vacuumLightFrame+1)%4;
					vacuumLightClock.restart();
				}
			}
			else
			{
				isVacuuming=false;
				vacuumLightFrame=0;
				vacuum_direction=0;
			}
		}

		if(show_suck_anim && suckAnimClock.getElapsedTime().asMilliseconds()>=500)
			show_suck_anim=false;

		for(int d=0;d<MAX_DEATH_ANIMS;d++)
		{
			if(death_active[d])
			{
				if(death_timer[d].getElapsedTime().asMilliseconds()>100)
				{
					death_frame[d]++;
					death_timer[d].restart();

					if(death_frame[d]>=6)
						death_active[d]=false;
				}
			}
		}

		for(int i=0;i<MAX_PROJECTILES;i++)
		{
			if(proj_active[i] && proj_ball_clock[i].getElapsedTime().asMilliseconds()>80)
			{
				proj_ball_frame[i]=(proj_ball_frame[i]+1)%4;
				proj_ball_clock[i].restart();
			}
		}

		if(!levelTransition)
		{
			update_walking_enemies(ghost_x,ghost_y,ghost_speed,ghost_direction,ghost_pause_clock,ghost_paused,ghost_pause_duration,ghostSprites,ghostTexture,ghostRightTex,ghost_active,num_ghosts,128);

			update_walking_enemies(chelnov_x,chelnov_y,chelnov_speed,chelnov_direction,chelnov_pause_clock,chelnov_paused,chelnov_pause_duration,chelnovSprites,chelnovLeftTex,chelnovRightTex,chelnov_active,num_chelnovs,128);

			update_skeletons(invisibleman_x,invisibleman_y,invisibleman_speed,invisibleman_direction,invisibleman_pause_clock,invisibleman_paused,invisibleman_pause_duration,invisibleman_velocityY,invisibleman_onGround,lvl,cell_size,invisibleman_active,num_invisiblemen);

			update_skeletons(skeleton_x,skeleton_y,skeleton_speed,skeleton_direction,skeleton_pause_clock,skeleton_paused,skeleton_pause_duration,skeleton_velocityY,skeleton_onGround,lvl,cell_size,skeleton_active,num_skeletons);

			update_projectiles(proj_x,proj_y,proj_vx,proj_vy,proj_type,proj_active,proj_stunned,proj_rolling,proj_roll_dir,proj_roll_speed,proj_stun_timer,projSprites,projBallTex,lvl,cell_size,proj_count,proj_dir,MAX_PROJECTILES);

			check_projectile_enemy_collision(proj_x,proj_y,proj_type,proj_active,ghost_x,ghost_y,ghost_active,num_ghosts,chelnov_x,chelnov_y,chelnov_active,num_chelnovs,invisibleman_x,invisibleman_y,invisibleman_active,num_invisiblemen,skeleton_x,skeleton_y,skeleton_active,num_skeletons,death_x,death_y,death_active,death_timer,death_frame,death_type,player_score,kills_in_window,multikill_timer,MAX_PROJECTILES,MAX_DEATH_ANIMS);

			if(powerup_spawn_clock.getElapsedTime().asSeconds()>=POWERUP_SPAWN_INTERVAL)
			{
				spawn_powerup(powerup_x,powerup_y,powerup_type,powerup_active,MAX_POWERUPS,lvl,height,width,cell_size);
				powerup_spawn_clock.restart();
			}

			check_powerup_collision(player_x,player_y,PlayerWidth,PlayerHeight,powerup_x,powerup_y,powerup_type,powerup_active,MAX_POWERUPS,POWERUP_SIZE,player_lives,speed_boost_active,speed_boost_timer,speed,base_speed);

			if(kills_in_window>=2 && !multikill_awarded)
			{
				if(multikill_timer.getElapsedTime().asSeconds()<multikill_window)
				{
					if(kills_in_window==2)
					{
						player_score+=200;
						multikill_awarded=true;
					}
					else if(kills_in_window>=3)
					{
						player_score+=500;
						multikill_awarded=true;
					}
				}
			}

			if(multikill_timer.getElapsedTime().asSeconds()>=multikill_window)
			{
				kills_in_window=0;
				multikill_awarded=false;
			}

			if(vacuum_burst_used)
			{
				player_score+=300;
				vacuum_burst_used=false;
			}

			if(speed_boost_active)
			{
				if(speed_boost_timer.getElapsedTime().asSeconds()>=SPEED_BOOST_DURATION)
				{
					speed_boost_active=false;
					speed=base_speed;
				}
			}

			if(!player_invincible && !player_dying)
			{
				for(int i=0;i<num_ghosts;i++)
				{
					if(ghost_active[i] && check_player_enemy_collision(player_x,player_y,PlayerWidth,PlayerHeight,ghost_x[i],ghost_y[i],128,128))
					{
						handlePlayerHit(player_lives,player_score,player_dying,player_death_frame,player_death_timer,player_x,player_y,respawn_x,respawn_y,velocityY,onGround,player_invincible,invincibility_clock);
						break;
					}
				}

				if(!player_dying && !player_invincible)
				{
					for(int i=0;i<num_chelnovs;i++)
					{
						if(chelnov_active[i] && check_player_enemy_collision(player_x,player_y,PlayerWidth,PlayerHeight,chelnov_x[i],chelnov_y[i],128,128))
						{
							handlePlayerHit(player_lives,player_score,player_dying,player_death_frame,player_death_timer,player_x,player_y,respawn_x,respawn_y,velocityY,onGround,player_invincible,invincibility_clock);
							break;
						}
					}
				}

				if(!player_dying && !player_invincible)
				{
					for(int i=0;i<num_invisiblemen;i++)
					{
						if(invisibleman_active[i] && check_player_enemy_collision(player_x,player_y,PlayerWidth,PlayerHeight,invisibleman_x[i],invisibleman_y[i],64,104))
						{
							handlePlayerHit(player_lives,player_score,player_dying,player_death_frame,player_death_timer,player_x,player_y,respawn_x,respawn_y,velocityY,onGround,player_invincible,invincibility_clock);
							break;
						}
					}
				}

				if(!player_dying && !player_invincible)
				{
					for(int i=0;i<num_skeletons;i++)
					{
						if(skeleton_active[i] && check_player_enemy_collision(player_x,player_y,PlayerWidth,PlayerHeight,skeleton_x[i],skeleton_y[i],64,104))
						{
							handlePlayerHit(player_lives,player_score,player_dying,player_death_frame,player_death_timer,player_x,player_y,respawn_x,respawn_y,velocityY,onGround,player_invincible,invincibility_clock);
							break;
						}
					}
				}
			}

			if(player_dying)
			{
				if(player_death_timer.getElapsedTime().asMilliseconds()>150)
				{
					player_death_frame++;
					player_death_timer.restart();

					if(player_death_frame>=6)
						window.close();
				}
			}

			if(player_invincible)
			{
				if(invincibility_clock.getElapsedTime().asSeconds()>=invincibility_duration)
					player_invincible=false;
			}

			if(!player_dying)
			{
				player_gravity(lvl,offset_y,velocityY,onGround,gravity,terminal_Velocity,player_x,player_y,cell_size,PlayerHeight,PlayerWidth);
			}
		}

		scoreText.setString("SCORE: "+to_string(player_score));

		if(speed_boost_active)
		{
			float time_left=SPEED_BOOST_DURATION-speed_boost_timer.getElapsedTime().asSeconds();
			speedBoostText.setString("SPEED BOOST: "+to_string((int)time_left)+"s");
		}

		window.clear();

		if(currentState==LEVEL1)
		{
			display_level(window,lvl,bgTex,bgSprite,blockTexture,blockSprite,height,width,cell_size);
		}
		else if(currentState==LEVEL2)
		{
			display_level2(window,bgTex2,bgSprite2,blockTexture,blockSprite,lvl,height,width,cell_size);
		}

		if(!player_dying && !levelTransition)
		{
			PlayerSprite.setPosition(player_x,player_y);

			if(!moving && !isVacuuming)
			{
				if(player_facing>0)
					PlayerSprite.setTexture(runRightTexture);
				else
					PlayerSprite.setTexture(runLeftTexture);
			}

			if(!player_invincible || (int)(invincibility_clock.getElapsedTime().asMilliseconds()/100)%2==0)
				window.draw(PlayerSprite);
		}
		else
		{
			PlayerSprite.setPosition(player_x,player_y);
			window.draw(PlayerSprite);
		}

		if(isVacuuming && !player_dying && !levelTransition)
		{
			if(vacuum_direction==1)
			{
				vacuumLightSpriteRight.setPosition(player_x+PlayerWidth+20,player_y+PlayerHeight/2-32);
				window.draw(vacuumLightSpriteRight);
			}
			else if(vacuum_direction==-1)
			{
				vacuumLightSpriteLeft.setPosition(player_x-100,player_y+PlayerHeight/2-32);
				window.draw(vacuumLightSpriteLeft);
			}
			else if(vacuum_direction==2)
			{
				vacuumLightSpriteDown.setPosition(player_x+PlayerWidth/2-32,player_y+PlayerHeight+20);
				window.draw(vacuumLightSpriteDown);
			}
			else if(vacuum_direction==-2)
			{
				vacuumLightSpriteUp.setPosition(player_x+PlayerWidth/2-32,player_y-100);
				window.draw(vacuumLightSpriteUp);
			}
		}

		if(show_suck_anim && !levelTransition)
		{
			if(suck_anim_type=='G' || suck_anim_type=='C')
			{
				suckSprite.setTexture(ghostSuckTexture);
				suckSprite.setScale(2,2);
			}
			else
			{
				suckSprite.setTexture(skeletonSuckTexture);
				suckSprite.setScale(0.3,0.3);
			}
			suckSprite.setPosition(suck_anim_x,suck_anim_y);
			window.draw(suckSprite);
		}

		for(int i=0;i<num_ghosts;i++)
		{
			if(ghost_active[i])
			{
				ghostSprites[i].setPosition(ghost_x[i],ghost_y[i]);
				window.draw(ghostSprites[i]);
			}
		}

		for(int i=0;i<num_chelnovs;i++)
		{
			if(chelnov_active[i])
			{
				chelnovSprites[i].setPosition(chelnov_x[i],chelnov_y[i]);
				window.draw(chelnovSprites[i]);
			}
		}

		for(int i=0;i<num_invisiblemen;i++)
		{
			if(invisibleman_active[i])
			{
				invisiblemanSprites[i].setPosition(invisibleman_x[i],invisibleman_y[i]);
				window.draw(invisiblemanSprites[i]);
			}
		}

		for(int i=0;i<num_skeletons;i++)
		{
			if(skeleton_active[i])
			{
				skeletonSprites[i].setPosition(skeleton_x[i],skeleton_y[i]);
				window.draw(skeletonSprites[i]);
			}
		}

		for(int i=0;i<MAX_PROJECTILES;i++)
		{
			if(proj_active[i])
				window.draw(projSprites[i]);
		}

		for(int d=0;d<MAX_DEATH_ANIMS;d++)
		{
			if(death_active[d])
			{
				if(death_type[d]=='G' || death_type[d]=='C')
				{
					deathSprites[d].setTexture(ghostDeathTex);
				}
				else
				{
					deathSprites[d].setTexture(skeletonDeathTex);
				}
				deathSprites[d].setPosition(death_x[d],death_y[d]);
				window.draw(deathSprites[d]);
			}
		}

		for(int i=0;i<MAX_POWERUPS;i++)
		{
			if(powerup_active[i])
			{
				if(powerup_type[i]=='S')
					powerup_sprites[i].setTexture(speedPowerupTex);
				else
					powerup_sprites[i].setTexture(healthPowerupTex);

				powerup_sprites[i].setPosition(powerup_x[i],powerup_y[i]);
				window.draw(powerup_sprites[i]);
			}
		}

		window.draw(playerFaceSprite);

		for(int i=0;i<player_lives && i<3;i++)
		{
			window.draw(heartSprites[i]);
		}

		window.draw(scoreText);
		window.draw(levelText);

		if(speed_boost_active)
		{
			window.draw(speedBoostText);
		}

		if(levelTransition)
		{
			RectangleShape transitionOverlay(Vector2f(1136,896));
			transitionOverlay.setFillColor(Color(0,0,0,150));
			window.draw(transitionOverlay);

			Text transitionText;
			transitionText.setFont(gameFont);
			transitionText.setCharacterSize(72);
			transitionText.setFillColor(Color::Yellow);
			transitionText.setOutlineColor(Color::Black);
			transitionText.setOutlineThickness(3);
			transitionText.setString(levelText.getString());

			FloatRect textBounds=transitionText.getLocalBounds();
			transitionText.setPosition(1136/2-textBounds.width/2,896/2-textBounds.height/2);
			window.draw(transitionText);

			if(currentState==LEVEL1)
			{
				Text bonusText;
				bonusText.setFont(gameFont);
				bonusText.setCharacterSize(48);
				bonusText.setFillColor(Color::Green);
				bonusText.setOutlineColor(Color::Black);
				bonusText.setOutlineThickness(2);
				bonusText.setString("BONUS: +1000 POINTS");

				FloatRect bonusBounds=bonusText.getLocalBounds();
				bonusText.setPosition(1136/2-bonusBounds.width/2,896/2-bonusBounds.height/2+80);
				window.draw(bonusText);
			}
		}

		window.display();
	}

	lvlMusic.stop();
	for(int i=0;i<height;i++)
	{
		delete[] lvl[i];
	}
	delete[] lvl;

	return 0;
}
