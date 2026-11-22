// Read-only verification through installed 7H serializers and IRO parser.
using System;using System.IO;using System.Reflection;using System.Xml.Serialization;
class PublicTest{
 static int checks;static void Check(bool v,string s){++checks;if(!v)throw new Exception(s);}
 static object Get(object o,string n){return o.GetType().GetProperty(n).GetValue(o,null);}
 static object Construct(Type t,string path){var c=t.GetConstructors()[0];var p=c.GetParameters();object[] a=new object[p.Length];for(int i=0;i<a.Length;++i)a[i]=i==0?(object)path:p[i].HasDefaultValue?p[i].DefaultValue:p[i].ParameterType==typeof(bool)?(object)false:null;return c.Invoke(a);}
 static void Validate(object m){
  Check((string)Get(m,"Author")=="Willard82","author");
  Check((string)Get(m,"DonationLink")=="https://ko-fi.com/ffviijobs","support URL");
  Check((string)Get(m,"Link")=="https://github.com/WailSolaiman/ffviijobs","project URL");
  Check(Get(m,"ID").ToString()=="2a737e43-7bcd-4da4-962d-3c87631fca5e","stable mod ID");
 }
 static int Main(string[] args){try{
  string program=Path.GetFullPath(args[0]),root=Path.GetFullPath(args[1]);
  AppDomain.CurrentDomain.AssemblyResolve+=delegate(object s,ResolveEventArgs e){string f=Path.Combine(program,new AssemblyName(e.Name).Name+".dll");return File.Exists(f)?Assembly.LoadFrom(f):null;};
  var w=Assembly.LoadFrom(Path.Combine(program,"AppWrapper.dll"));var core=Assembly.LoadFrom(Path.Combine(program,"AppCore.dll"));
  object info=Construct(w.GetType("AppWrapper.ModInfo"),Path.Combine(root,"mod/mod.xml"));
  // Runtime ModInfo does not populate DonationLink. The importer handles it.
  // Parse metadata in a private folder; omit preview there to avoid touching
  // the application's image cache. Actual IRO preview is checked separately.
  string fixture=Path.GetFullPath(args[2]);Directory.CreateDirectory(fixture);
  var doc=new System.Xml.XmlDocument();doc.Load(Path.Combine(root,"mod/mod.xml"));
  var pv=doc.SelectSingleNode("/ModInfo/PreviewFile");pv.ParentNode.RemoveChild(pv);doc.Save(Path.Combine(fixture,"mod.xml"));
  var importer=Activator.CreateInstance(core.GetType("AppCore.ModImporter"));
  object parsed=importer.GetType().GetMethod("ParseModXmlFromSource").Invoke(importer,new object[]{fixture,null});Validate(parsed);
  Check((decimal)Get(info,"Version")==0.80m,"package version");Check((string)Get(info,"PreviewFile")=="preview.png","embedded preview path");
  object entry;var serializer=new XmlSerializer(core.GetType("Iros.Workshop.Mod"));
  using(var f=File.OpenRead(Path.Combine(root,"catalog-submission/mods/qhimm/FFVII Job System/mod.xml")))entry=serializer.Deserialize(f);
  Validate(entry);object version=Get(entry,"LatestVersion");
  Check((decimal)Get(version,"Version")==0.80m,"catalogue version");
  Check(Get(version,"CompatibleGameVersions").ToString()=="Steam","tested game edition");
  Check((int)Get(version,"DownloadSize")== (int)Math.Ceiling(new FileInfo(Path.Combine(root,"FFVIIJobSystem-0.8.0.iro")).Length/1024.0),"download size in KB");
  Check((string)Get(version,"PreviewImage")=="https://github.com/WailSolaiman/ffviijobs/releases/download/v0.8.0/preview.png","staged release preview URL");
  object archive=Construct(w.GetType("AppWrapper.IrosArc"),Path.Combine(root,"FFVIIJobSystem-0.8.0.iro"));
  byte[] art=(byte[])archive.GetType().GetMethod("GetBytes").Invoke(archive,new object[]{"preview.png"});
  Check(Convert.ToBase64String(art)==Convert.ToBase64String(File.ReadAllBytes(Path.Combine(root,"preview.png"))),"IRO contains the new class-roster preview");
  Console.WriteLine("PASS "+checks+" installed-7H public package/catalogue metadata checks. Remote links are staged, not verified live.");return 0;
 }catch(Exception e){Console.WriteLine(e);return 1;}}
}
