using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Serialization;

namespace pantherScripts
{
    /*
     * 
     * <MCT>
<Catalogs>
<Catalog version="1.4.1">
<PublishedMedia id="" release="">
<Files>
<File id="">
<FileName>19045.3803.231204-0204.22h2_release_svc_refresh_CLIENTCHINA_RET_x64FRE_zh-cn.esd</FileName>
<LanguageCode>zh-cn</LanguageCode>
<Language>Chinese (Simplified, China)</Language>
<Edition>CoreCountrySpecific</Edition>
<Architecture>x64</Architecture>
<Size>3945834799</Size>
<Sha1>8b49a8943cb3260ce9a8dadcd729f0ac98018245</Sha1>
<FilePath>http://dl.delivery.mp.microsoft.com/filestreamingservice/files/6048ac73-c010-4eaf-ac07-a8672588662e/19045.3803.231204-0204.22h2_release_svc_refresh_CLIENTCHINA_RET_x64FRE_zh-cn.esd</FilePath>
<Key/>
<Architecture_Loc>%ARCH_64%</Architecture_Loc>
<Edition_Loc>%BASE_CHINA%</Edition_Loc>
<IsRetailOnly>False</IsRetailOnly>
</File>
    */

    // Root element: <MCT>
    [XmlRoot("MCT")]
    public class MctCatalog
    {
        [XmlArray("Catalogs")]
        [XmlArrayItem("Catalog")]
        public List<Catalog> Catalogs { get; set; }
    }

    public class Catalog
    {
        [XmlAttribute("version")]
        public string Version { get; set; }

        [XmlElement("PublishedMedia")]
        public List<PublishedMedia> PublishedMedia { get; set; }
    }

    public class PublishedMedia
    {
        [XmlAttribute("id")]
        public string Id { get; set; }

        [XmlAttribute("release")]
        public string Release { get; set; }

        [XmlArray("Files")]
        [XmlArrayItem("File")]
        public List<EsdFile> Files { get; set; }
    }

    public class EsdFile
    {
        [XmlAttribute("id")]
        public string Id { get; set; }

        [XmlElement("FileName")]
        public string FileName { get; set; }

        [XmlElement("LanguageCode")]
        public string LanguageCode { get; set; }

        [XmlElement("Language")]
        public string Language { get; set; }

        [XmlElement("Edition")]
        public string Edition { get; set; }

        [XmlElement("Architecture")]
        public string Architecture { get; set; }

        [XmlElement("Size")]
        public long Size { get; set; }

        [XmlElement("Sha1")]
        public string Sha1 { get; set; }

        [XmlElement("FilePath")]
        public string FilePath { get; set; }

        [XmlElement("Key")]
        public string Key { get; set; }

        [XmlElement("Architecture_Loc")]
        public string ArchitectureLoc { get; set; }

        [XmlElement("Edition_Loc")]
        public string EditionLoc { get; set; }

        [XmlElement("IsRetailOnly")]
        public string IsRetailOnly { get; set; }
    }
}
