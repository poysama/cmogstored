# this is only used for release-related tasks and should not
# be needed by normal development

require "nokogiri" # gem install nokogiri
include Rake::DSL if defined?(Rake::DSL)

url_base = "http://bogomips.org/cmogstored"
cgit_url = url_base + '.git'
git_url = 'git://bogomips.org/cmogstored.git'

def tags
  timefmt = '%Y-%m-%dT%H:%M:%SZ'
  @tags ||= `git tag -l`.split(/\n/).map do |tag|
    if %r{\Av[\d\.]+} =~ tag
      header, subject, body = `git cat-file tag #{tag}`.split(/\n\n/, 3)
      header = header.split(/\n/)
      tagger = header.grep(/\Atagger /).first
      body ||= "initial"
      {
        :time => Time.at(tagger.split(/ /)[-2].to_i).utc.strftime(timefmt),
        :tagger_name => %r{^tagger ([^<]+)}.match(tagger)[1].strip,
        :tagger_email => %r{<([^>]+)>}.match(tagger)[1].strip,
        :id => `git rev-parse refs/tags/#{tag}`.chomp!,
        :tag => tag,
        :subject => subject,
        :body => body,
      }
    end
  end.compact.sort { |a,b| b[:time] <=> a[:time] }
end

desc 'prints news as an Atom feed'
task :news_atom do
  require 'nokogiri'
  new_tags = tags[0,10]
  puts(Nokogiri::XML::Builder.new do
    feed :xmlns => "http://www.w3.org/2005/Atom" do
      id! "#{url_base}/NEWS.atom.xml"
      title "cmogstored news"
      subtitle "alternative mogstored implementation for MogileFS"
      link! :rel => 'alternate', :type => 'text/plain',
            :href => "#{url_base}/NEWS"
      updated(new_tags.empty? ? "1970-01-01T00:00:00Z" : new_tags.first[:time])
      new_tags.each do |tag|
        entry do
          title tag[:subject]
          updated tag[:time]
          published tag[:time]
          author {
            name tag[:tagger_name]
            email tag[:tagger_email]
          }
          url = "#{cgit_url}/tag/?id=#{tag[:tag]}"
          link! :rel => "alternate", :type => "text/html", :href =>url
          id! url
          message_only = tag[:body].split(/\n.+\(\d+\):\n {6}/).first.strip
          content({:type =>:text}, message_only)
          content(:type =>:xhtml) { pre tag[:body] }
        end
      end
    end
  end.to_xml)
end

desc 'prints news as a text file'
task :news do
  tags.each do |tag|
    time = tag[:time].tr!('T', ' ').gsub!(/:\d\dZ/, ' UTC')
    line = "#{tag[:tag].sub(/^v/, '')} / #{time}"
    puts line
    puts("-" * line.length)
    puts ""

    puts tag[:body].gsub(/^/m, "  ").gsub(/[ \t]+$/m, "")
    puts "" unless tag == tags.last
  end
end

desc "dump changelog to stdout"
task :changelog do
  since = "v0.1.0"

  puts "cmogstored changelog since #{since}"
  puts "Full changeset information is available at #{git_url}"
  puts "See NEWS file for a user-oriented summary of changes"
  puts ""

  cmd = %w(git log --pretty=medium --date=iso --decorate) << "#{since}.."
  system(*cmd) or abort $?
end

desc "compare dist tar.gz against contents in the git tree"
task :distcheck_git do
  tgz = ENV["TGZ"] or abort "TGZ= not specified"
  tgzfiles = `tar -ztf #{tgz}`.split(/\n/)
  tgzfiles.map! { |f| f.gsub!(%r{^[^/]+/}, '') }
  gitfiles = `git ls-files`.split(/\n/)
  gitonly = gitfiles - tgzfiles
  gitonly -= %w(build-aux/manpage-hack.mk)
  if gitonly[0]
    warn "The following files are missing from #{tgz}"
    warn ""
    gitonly.each { |f| warn f }
    exit(1)
  end
end

desc "post to fm"
task :fm_update do
  require 'tempfile'
  require 'net/http'
  require 'net/netrc'
  require 'json'
  version = ENV['VERSION'] or abort "VERSION= needed"
  uri = URI.parse('https://freecode.com/projects/cmogstored/releases.json')
  rc = Net::Netrc.locate('cmogstored-fm') or
    abort "~/.netrc or entry not found"
  api_token = rc.password
  _, subject, body = `git cat-file tag v#{version}`.split(/\n\n/, 3)
  tmp = Tempfile.new('fm-changelog')
  tmp.puts subject
  tmp.puts
  tmp.puts body
  tmp.flush
  system(ENV["VISUAL"], tmp.path) or abort "#{ENV["VISUAL"]} failed: #$?"
  changelog = File.read(tmp.path).strip

  req = {
    "auth_code" => api_token,
    "release" => {
      "tag_list" => "stable",
      "version" => version,
      "changelog" => changelog,
    },
  }.to_json

  if ! changelog.strip.empty? && version =~ %r{\A[\d\.]+\d+\z}
    Net::HTTP.start(uri.host, uri.port, :use_ssl => true) do |http|
      p http.post(uri.path, req, {'Content-Type'=>'application/json'})
    end
  else
    warn "not updating freshmeat for v#{version}"
  end
end
