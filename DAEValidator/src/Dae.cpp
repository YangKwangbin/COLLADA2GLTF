
#include "Dae.h"
#include "StringUtil.h"
#include "Strings.h"
#include <sstream>

using namespace std;

namespace opencollada
{
	const string xpath_all = "//collada:";
	const string xpath_or_all = "|//collada:";
	const string xpath_child = "/collada:";

	Dae::Dae(Dae && other)
	{
		*this = move(other);
	}

	const Dae & Dae::operator = (Dae && other)
	{
		this->XmlDoc::operator=(move(other));
		mUri = move(other.mUri);
		mExternalDAEs = move(other.mExternalDAEs);
		return *this;
	}

	void Dae::readFile(const string & path)
	{
		mUri = Uri::FromNativePath(path);

		Super::readFile(path);

		if (!*this)
			return;

		// List referenced DAEs

		// InstanceWithExtra and other <instance_*> with "url" attribute
		auto instances = root().selectNodes(
			xpath_all + Strings::instance_animation +
			xpath_or_all + Strings::instance_camera +
			xpath_or_all + Strings::instance_controller +
			xpath_or_all + Strings::instance_effect +
			xpath_or_all + Strings::instance_force_field +
			xpath_or_all + Strings::instance_formula +
			xpath_or_all + Strings::instance_geometry +
			xpath_or_all + Strings::instance_image +
			xpath_or_all + Strings::instance_joint +
			xpath_or_all + Strings::instance_kinematics_model +
			xpath_or_all + Strings::instance_kinematics_scene +
			xpath_or_all + Strings::instance_light +
			xpath_or_all + Strings::instance_node +
			xpath_or_all + Strings::instance_physics_material +
			xpath_or_all + Strings::instance_physics_model +
			xpath_or_all + Strings::instance_physics_scene +
			xpath_or_all + Strings::instance_visual_scene
		);
		for (auto instance : instances)
			if (auto url = instance.attribute(Strings::url))
				onAnyDAEURI(instance.line(), url.value());

		// <instance_node>
		auto instance_nodes = root().selectNodes(xpath_all + Strings::instance_node);
		for (auto instance_node : instance_nodes)
		{
			if (auto proxy = instance_node.attribute(Strings::proxy))
				onAnyDAEURI(instance_node.line(), proxy.value());
		}

		// <accessor>
		// <skin>
		// <morph>
		auto elementsWithSource = root().selectNodes(
			xpath_all + Strings::accessor +
			xpath_or_all + Strings::skin +
			xpath_or_all + Strings::morph
		);
		for (auto element : elementsWithSource)
		{
			if (auto source = element.attribute(Strings::source))
				onAnyDAEURI(element.line(), source.value());
		}

		// <render>
		auto renders = root().selectNodes(xpath_all + Strings::render);
		for (auto render : renders)
		{
			if (auto camera_node = render.attribute("camera_node"))
				onAnyDAEURI(render.line(), camera_node.value());
		}

		// <skeleton>
		auto skeletons = root().selectNodes(xpath_all + Strings::skeleton);
		for (auto skeleton : skeletons)
		{
			onAnyDAEURI(skeleton.line(), skeleton.text());
		}

		// <instance_material>
		// <instance_rigid_body>
		auto elementsWithTarget = root().selectNodes(
			xpath_all + Strings::instance_material +
			xpath_or_all + Strings::instance_rigid_body
		);
		for (auto element : elementsWithTarget)
		{
			if (auto target = element.attribute(Strings::target))
				onAnyDAEURI(element.line(), target.value());
		}

		// <instance_physics_model>
		auto instance_physics_models = root().selectNodes(xpath_all + Strings::instance_physics_model);
		for (auto instance_physics_model : instance_physics_models)
		{
			if (auto parent = instance_physics_model.attribute(Strings::parent))
				onAnyDAEURI(instance_physics_model.line(),  parent.value());
		}

		// <convex_mesh>
		auto convex_meshes = root().selectNodes(xpath_all + Strings::convex_mesh);
		for (auto convex_mesh : convex_meshes)
		{
			if (auto convex_hull_of = convex_mesh.attribute(Strings::convex_hull_of))
				onAnyDAEURI(convex_mesh.line(), convex_hull_of.value());
		}

		// TODO
		// rigid_body attribute is an sid. Make sid check.

		// <ref_attachment>
		// <attachment>
		//auto attachments = root().selectNodes(
		//	xpath_all + Strings::attachment +
		//	xpath_or_all + Strings::ref_attachment
		//);
		//for (auto attachment : attachments)
		//{
		//	if (auto rigid_body = attachment.attribute(Strings::rigid_body))
		//		onAnyDAEURI(attachment.line(), rigid_body.value());
		//}

		// Load found DAE references
		for (auto & uri_dae : mExternalDAEs)
			uri_dae.second.readExternalFile(uri_dae.first.nativePath());

		// Look for additional xs:anyURI that are not DAE references:

		// <image>/<init_from>
		auto init_froms = root().selectNodes(xpath_all + Strings::image + xpath_child + Strings::init_from);
		for (auto init_from : init_froms)
		{
			onAnyURI(init_from.line(), init_from.text());
		}

		// <binary>/<ref>
		// <init_from>/<ref>
		auto refs = root().selectNodes(
			xpath_all + Strings::binary + xpath_child + Strings::ref +
			xpath_or_all + Strings::init_from + xpath_child + Strings::ref
		);
		for (auto ref : refs)
		{
			onAnyURI(ref.line(), ref.text());
		}

		// <include>
		// <profile_BRIDGE>
		auto elementsWithUrl = root().selectNodes(
			xpath_all + Strings::include +
			xpath_or_all + Strings::profile_BRIDGE
		);
		for (auto element : elementsWithUrl)
		{
			if (auto url = element.attribute(Strings::url))
				onAnyURI(element.line(), url.value());
		}

		// Ignore these anyURI:
		// <COLLADA>@base
		// <contributor>/<author_website>
		// <contributor>/<source_data>

		// Look for URI fragments:

		// <channel>
		// <input>
		auto elementsWithSourceFragment = root().selectNodes(
			xpath_all + Strings::channel +
			xpath_or_all + Strings::input
		);
		for (auto element : elementsWithSourceFragment)
		{
			if (auto source = element.attribute(Strings::source))
				//onURIFragment(element.line(), source.value());
				onAnyURI(element.line(), source.value());
		}

		// Look for IDREFs

		// <IDREF_array>
		auto IDREF_arrays = root().selectNodes(xpath_all + Strings::IDREF_array);
		for (auto IDREF_array : IDREF_arrays)
		{
			stringstream ss(IDREF_array.text());
			string buffer;
			while (ss >> buffer)
				mIDREFs.emplace_back(IDREF_array.line(), buffer);
		}

		// <accessor>/<param type="IDREF">
		auto params = root().selectNodes(xpath_all + Strings::accessor + xpath_child + Strings::param + "[@type=\"IDREF\"]");
		for (auto param : params)
		{
			mIDREFs.emplace_back(param.line(), param.text());
		}
	}

	void Dae::initializeIdCache() const
	{
		if (!mIdCacheInitialized)
		{
			mIdCacheInitialized = true;

			auto nodes = root().selectNodes("//*[@id]");
			for (auto node : nodes)
			{
				string id = node.attribute("id").value();
				mIdCache.insert(id);
				mLineIdCache.emplace_back(node.line(), id);
			}
		}
	}

	const std::set<string> & Dae::getIds() const
	{
		initializeIdCache();
		return mIdCache;
	}

	const std::vector<std::tuple<size_t, std::string>> & Dae::getLineIds() const
	{
		initializeIdCache();
		return mLineIdCache;
	}

	void Dae::readExternalFile(const string & url)
	{
		// Simple load file but not the references
		Super::readFile(url);
	}

	void Dae::onAnyDAEURI(size_t line, const string & uri)
	{
		onAnyURI(line, uri);

		if (String::StartsWith(uri, "#"))
			return;

		Uri absoluteUri(mUri, uri);

		if (absoluteUri.path().empty())
			return;

		if (!absoluteUri.pathFile().empty())
		{
			Uri absoluteUriNoFragment(absoluteUri);
			absoluteUriNoFragment.setFragment(string());
			if (absoluteUriNoFragment != mUri)
			{
				mExternalDAEs[absoluteUriNoFragment] = Dae();
			}
		}
	}

	void Dae::onAnyURI(size_t line, const std::string & uri)
	{
		mAnyURIs.emplace_back(line, Uri(mUri, uri));
	}
}