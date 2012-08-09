/* === S Y N F I G ========================================================= */
/*!	\file synfigapp/instance.cpp
**	\brief Instance
**
**	$Id$
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**	Copyright (c) 2007, 2008 Chris Moore
**  Copyright (c) 2011 Carlos López
**
**	This package is free software; you can redistribute it and/or
**	modify it under the terms of the GNU General Public License as
**	published by the Free Software Foundation; either version 2 of
**	the License, or (at your option) any later version.
**
**	This package is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
**	General Public License for more details.
**	\endlegal
*/
/* ========================================================================= */

/* === H E A D E R S ======================================================= */

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "instance.h"
#include "canvasinterface.h"
#include <iostream>
#include <synfig/loadcanvas.h>
#include <synfig/savecanvas.h>
#include <synfig/valuenode_composite.h>
#include <synfig/valuenode_radialcomposite.h>
#include <synfig/valuenode_reference.h>
#include <synfig/valuenode_greyed.h>
#include <synfig/valuenode_blinecalctangent.h>
#include <synfig/valuenode_blinecalcvertex.h>
#include <synfig/valuenode_blinecalcwidth.h>
#include <synfig/valuenode_wplist.h>
#include <synfig/valuenode_scale.h>
#include <synfig/valuenode_range.h>
#include <synfig/layer_pastecanvas.h>
#include <map>

#include <zip.h>
#include <libgen.h>
#include <string>
#include <string.h>

#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>



#include "general.h"

#endif

/* === U S I N G =========================================================== */

using namespace std;
using namespace etl;
using namespace synfig;
using namespace synfigapp;

/* === M A C R O S ========================================================= */

/* === G L O B A L S ======================================================= */

static std::map<loose_handle<Canvas>, loose_handle<Instance> > instance_map_;

/* === P R O C E D U R E S ================================================= */

bool
synfigapp::is_editable(synfig::ValueNode::Handle value_node)
{
	if(ValueNode_Const::Handle::cast_dynamic(value_node)
		|| ValueNode_Animated::Handle::cast_dynamic(value_node)
		|| ValueNode_Composite::Handle::cast_dynamic(value_node)
		|| ValueNode_RadialComposite::Handle::cast_dynamic(value_node)
		||(ValueNode_Reference::Handle::cast_dynamic(value_node) && !ValueNode_Greyed::Handle::cast_dynamic(value_node))
		|| ValueNode_BLineCalcVertex::Handle::cast_dynamic(value_node)
		|| ValueNode_BLineCalcTangent::Handle::cast_dynamic(value_node)
		|| ValueNode_BLineCalcWidth::Handle::cast_dynamic(value_node)
		|| ValueNode_Scale::Handle::cast_dynamic(value_node)
		|| ValueNode_Range::Handle::cast_dynamic(value_node)
	)
		return true;
	return false;
}

etl::handle<Instance>
synfigapp::find_instance(etl::handle<synfig::Canvas> canvas)
{
	if(instance_map_.count(canvas)==0)
		return 0;
	return instance_map_[canvas];
}

/* === M E T H O D S ======================================================= */

Instance::Instance(etl::handle<synfig::Canvas> canvas):
	CVSInfo(canvas->get_file_name()),
	canvas_(canvas)
{
	assert(canvas->is_root());

	unset_selection_manager();

	instance_map_[canvas]=this;
} // END of synfigapp::Instance::Instance()

handle<Instance>
Instance::create(etl::handle<synfig::Canvas> canvas)
{
	// Construct a new instance
	handle<Instance> instance(new Instance(canvas));

	return instance;
} // END of synfigapp::Instance::create()

synfig::String
Instance::get_file_name()const
{
	return get_canvas()->get_file_name();
}

void
Instance::set_file_name(const synfig::String &name)
{
	get_canvas()->set_file_name(name);
	CVSInfo::set_file_name(name);
}

Instance::~Instance()
{
	instance_map_.erase(canvas_);

	if (getenv("SYNFIG_DEBUG_DESTRUCTORS"))
		synfig::info("Instance::~Instance(): Deleted");
}

handle<CanvasInterface>
Instance::find_canvas_interface(synfig::Canvas::Handle canvas)
{
	if(!canvas)
		return 0;
	while(canvas->is_inline())
		canvas=canvas->parent();

	CanvasInterfaceList::iterator iter;

	for(iter=canvas_interface_list().begin();iter!=canvas_interface_list().end();iter++)
		if((*iter)->get_canvas()==canvas)
			return *iter;

	return CanvasInterface::create(this,canvas);
}

bool
Instance::save()
{
	std::map <std::string, bool> externals_list = canvas_->get_external_files_list();

	std::string project_dir = canvas_->get_project_dir();

	bool ret;

	if (filename_extension(get_file_name())==".sifp")
	{
	//std::map <std::string, std::string> images_map;
		
		// save as sif to temp dir

		
		ret=save_canvas(project_dir+ETL_DIRECTORY_SEPARATOR+"main.sif",canvas_);

		/*for (iter = externals_list.begin(); iter != externals_list.end(); iter++)
		{
			synfig::info("after: "+(*iter).first);
		} */
		
		// pack everything to zip
		save_sifp(get_file_name(), externals_list, project_dir+ETL_DIRECTORY_SEPARATOR);

		canvas_ -> external_files_list_reset();
		
	} else 
	
	ret=save_canvas(get_file_name(),canvas_);
	if(ret)
	{
		// if .sifp then pack all files to zip
		reset_action_count();
		const_cast<sigc::signal<void>& >(signal_saved_)();
	}
	return ret;
}

bool
Instance::save_as(const synfig::String &file_name)
{
	bool ret;

	String old_file_name(get_file_name());

	set_file_name(file_name);
	
	/* if (filename_extension(file_name) == ".sifp")
	{
		canvas_->get_external_files_list();
	} */
	//canvas_->get_external_files_list();

	//map <std::string, std::string> images_map;
	if (filename_extension(file_name) == ".sifp")
	{
		std::map <std::string, bool> externals_list = canvas_->get_external_files_list();
		
		std::map <std::string, bool>::iterator iter;
		for (iter = externals_list.begin(); iter != externals_list.end(); iter++)
		{
			std::string image_path;
			if ((*iter).second)
			{ 
				image_path = (*iter).first;

				char* image_path_char = new char[image_path.length()+1];
				strcpy(image_path_char, image_path.c_str());
			
				std::string image_name_char = basename(image_path_char);
				std::string image_name(image_name_char);
				std::string image_extension = filename_extension(image_name);
				image_name = filename_sans_extension(image_name);
				std::string image_name_n = image_name+image_extension;
				int count = 0;
				while (images_map.count(image_name_n)==1)
				{
					count++;
					std::string s(strprintf("%d",count));
					image_name_n = image_name+"_"+s+image_extension;
				}
				//synfig::info(image_path.c_str());
				images_map[image_name_n]=image_path;
			}
		}
	
		

		//std::string path =get_file_path ();
		update_path_for_zip(canvas_);
		
		static string charset ="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
		std::string rsuffix;
		int length = 12;
		rsuffix.resize(length);

		srand(time(NULL));

		for (int i = 0; i < length; i++)
				rsuffix[i] = charset[rand() % charset.length()];

		
		String tmp_dir;
		#ifdef WIN32
        	tmp_dir = String(getenv("HOMEDRIVE"))+ETL_DIRECTORY_SEPARATOR+getenv("HOMEPATH")+ETL_DIRECTORY_SEPARATOR+SYNFIG_USER_APP_DIR;
		#else
        	tmp_dir = String(getenv("HOME"))+ETL_DIRECTORY_SEPARATOR+SYNFIG_USER_APP_DIR;
		#endif
		tmp_dir = tmp_dir+ETL_DIRECTORY_SEPARATOR+"tmp"+ETL_DIRECTORY_SEPARATOR;
		char* file_name_char = new char[file_name.length()+1];
		strcpy(file_name_char, file_name.c_str());
		tmp_dir = tmp_dir+basename(file_name_char)+String("-"+rsuffix);
		synfig::info("tmp_dir: "+tmp_dir);

		canvas_->set_project_dir(tmp_dir);
				
		// copy files to temp dir
		std::map <std::string, std::string>::iterator iter2;
		
		mkdir((tmp_dir).c_str(), 0755);
				
		mkdir((tmp_dir + ETL_DIRECTORY_SEPARATOR+"images").c_str(),  0755);
		
		for (iter2=images_map.begin(); iter2!=images_map.end(); iter2++)
		{
			std::string image_name = (*iter2).second;
			
			std::ifstream  src(image_name.c_str());
			std::ofstream  dst((tmp_dir + ETL_DIRECTORY_SEPARATOR+"images"+ETL_DIRECTORY_SEPARATOR+(*iter2).first).c_str());

			dst << src.rdbuf();
		} 

		
		
		// save as sif to temp dir

		
		ret=save_canvas(tmp_dir+ETL_DIRECTORY_SEPARATOR+"main.sif",canvas_);

		update_externals_list(externals_list, images_map);

		for (iter = externals_list.begin(); iter != externals_list.end(); iter++)
		{
			synfig::info("after: "+(*iter).first);
		}
		
		// pack everything to zip
		save_sifp(file_name, externals_list, tmp_dir+ETL_DIRECTORY_SEPARATOR);
		
		canvas_ -> external_files_list_reset();
	} 
	else
		ret=save_canvas(file_name,canvas_);

	if(ret)
	{
		reset_action_count();
		signal_saved_();
	}
	else
		set_file_name(old_file_name);

	signal_filename_changed_();

	return ret;
}

void
Instance::update_externals_list (std::map<std::string, bool> &externals_list, std::map<std::string, std::string> images_map)
{
	std::map<std::string, std::string>::iterator iter;  
	for (iter = images_map.begin(); iter != images_map.end();  iter++)
	{
		externals_list.erase((*iter).second);
		externals_list[(*iter).first] = false;
	}
	//return externals_list;
}


void
Instance::save_sifp(std::string file_name, std::map <std::string, bool> externals_list, std::string path)
{
	struct zip *zip_archive;
	int err;
	zip_archive=zip_open(file_name.c_str(), ZIP_CREATE, &err);
	if (zip_archive == NULL) zip_archive=zip_open(file_name.c_str(), 0, &err);
	
	//ret=save_canvas_to_zip(file_name, canvas_, zip_archive); -- remove from core!!!
	std::map<std::string, bool>::iterator iter;
	for (iter=externals_list.begin(); iter!=externals_list.end(); iter++)
	{
	//	synfig::info(((*iter).first).c_str());
	//	synfig::info("|||||||||||||");
	//	synfig::info(((*iter).second).c_str());
		if ((*iter).second)
		{
			std::string image_name = path+"images"+ETL_DIRECTORY_SEPARATOR+(*iter).first;
			struct zip_source *zs;

			//std::string abspath (dir);
			//abspath = abspath+"/"+external;
		
			zs=zip_source_file(zip_archive,image_name.c_str(), 0, -1);
		
			//synfig::info("abspath");
			//synfig::info(image_name.c_str());
		
			//char* external_char = new char[external.length()+1];
			//strcpy(external_char, external.c_str());

			std::string img_dir ("images") ;
			std::string in_zip_path = img_dir+ETL_DIRECTORY_SEPARATOR+(*iter).first;
		//	in_zip_path = +in_zip_path;
		
			zip_add(zip_archive, in_zip_path.c_str(), zs);
		}
		
	}

	struct zip_source *zs;
	String path_main_sif = path+"main.sif";
	zs=zip_source_file(zip_archive, path_main_sif.c_str(), 0, -1);
	zip_add(zip_archive, "main.sif", zs);
	
	zip_close(zip_archive);
}

void
Instance::update_path_for_zip(synfig::Canvas::Handle canvas)
{
	for (Canvas::const_iterator iter = canvas->begin(); iter != canvas->end();  iter++)
	{
		const etl::handle<Layer> layer = *iter;
		std::string n;
		std::string path = canvas->get_file_path ();
		n = layer->get_name();
		//synfig::info(layer->get_description().c_str());
		if (n=="import")
		{
			ValueBase param = layer->get_param("filename");
			if(param.get_type()==ValueBase::TYPE_STRING) 
			{
				//ValueBase ret(ValueBase::TYPE_STRING);
				std::string param_s = param.get(String());
				std::string abspath = path+ETL_DIRECTORY_SEPARATOR+param_s;
				std::map<std::string,std::string>::iterator iter_images;
				for (iter_images=images_map.begin(); iter_images!=images_map.end(); iter_images++)
				{	
					if ((*iter_images).second==abspath)
					{
						std::string in_zip="#"+(*iter_images).first;
						layer->set_param("filename",ValueBase(in_zip));
					}
				}
				//layer->set_param("filename",  ValueBase(*iter_images));
				//std::string s = *iter_images;
				//synfig::info("++++++++++++++++");
				//synfig::info(abspath.c_str());
				//if (images_map.count(abspath)==1) synfig::info("1"); else synfig::info("0");
				//synfig::info(((*iter_images).first).c_str());
				//std::string filename_image;
				//filename_image=(*iter_images).first;
				//synfig::info("-----------------");
				//synfig::info(filename_image.c_str());
				//layer->set_param("filename",ValueBase(filename_image));
				//synfig::info("___");
				//std::list<std::string>::iterator iter2;
				//for (iter2 = external_image_list_.begin(); iter2!=external_image_list_.end(); iter2++)
				//{
				//	std::string fname = *iter2;
				//	synfig::info(fname.c_str());
				//}
				//synfig::info("___"); 
			} 
		} 
		if(n=="PasteCanvas")
		{
			synfig::Layer_PasteCanvas* paste_canvas(static_cast<synfig::Layer_PasteCanvas*>(layer.get()));
			synfig::Canvas::Handle paste_sub_canvas = paste_canvas->get_sub_canvas();
			if (paste_sub_canvas->is_inline())
				update_path_for_zip(paste_sub_canvas);
			else
				//update_path_for_zip(paste_sub_canvas);
				// TODO: Do nothing for now.
				synfig::info("%d",paste_sub_canvas -> size());
		} 
	} 					
}
